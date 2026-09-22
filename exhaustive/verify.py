#!/usr/bin/env python3
"""Verify this portable evidence package; never execute a search binary.

Python 3.10+, standard library only. Run without -O (archived checks use assert).
The original audit records are read-only. --report writes a new summary.
"""
import argparse
from collections import Counter
from datetime import datetime, timezone
import gzip
import hashlib
import importlib.util
from itertools import combinations
import json
from math import isqrt
from pathlib import Path
import re
import shutil
import sys
import tarfile
import tempfile
import time

ROOT = Path(__file__).resolve().parent
sys.dont_write_bytecode = True
RUNS = ('exhaustive-2026-09-16', 'exhaustive-2026-09-18',
        'exhaustive-2026-09-19', 'exhaustive-2026-09-20')
FRONTIER = 4_763_200_000_000
HISTORICAL_SHA = '1dca944413520b5cf1596c4c5820b454e1037c13177ff479d39435e5ee4d5301'
spec = importlib.util.spec_from_file_location(
    'archived_verifier', ROOT/'evidence/verify_continuation.original.py')
original = importlib.util.module_from_spec(spec)
spec.loader.exec_module(original)


def require(condition, message):
    if not condition:
        raise ValueError(message)


def digest(path):
    with Path(path).open('rb') as f:
        return hashlib.file_digest(f, 'sha256').hexdigest() if hasattr(hashlib, 'file_digest') else _digest(f)


def _digest(stream):
    h = hashlib.sha256()
    for chunk in iter(lambda: stream.read(1024*1024), b''):
        h.update(chunk)
    return h.hexdigest()


def square(n):
    return n >= 0 and isqrt(n)**2 == n


def prime_list(bound, residue=None):
    """Include enough initial primes that their product reaches bound."""
    result = []; product = 1; n = 2
    while product < bound:
        if all(n % d for d in range(2, isqrt(n)+1)) and (residue is None or n % 4 == residue):
            result.append(n); product *= n
        n += 1
    return result


def maximum_divisor_product(bound, primes):
    """Maximize product(e_i+1) for prod(p_i**e_i) < bound.

    Moving exponents onto smaller primes and sorting them in nonincreasing
    order can only lower the represented integer, so this finite DFS suffices.
    """
    best = (1, 1)
    def visit(i, cap, n, product):
        nonlocal best
        if product > best[0]:
            best = (product, n)
        if i == len(primes):
            return
        for exponent in range(1, cap+1):
            n *= primes[i]
            if n >= bound:
                break
            visit(i+1, exponent, n, product*(exponent+1))
    visit(0, bound.bit_length(), 1, 1)
    return best


def capacity_bounds():
    limit = 10**12
    divisors, witness_d = maximum_divisor_product(limit, prime_list(limit))
    product, witness_r = maximum_divisor_product(limit, prime_list(limit, 1))
    # Positive unordered representations p^2+q^2 are at most ceil(product/2).
    reps = (product+1)//2
    require(divisors < 16384 and reps < 512, 'Historical array capacity bound failed')
    return {'historical_S_exclusive': limit, 'maximum_divisors': divisors,
            'divisor_witness': witness_d, 'representation_upper_bound': reps,
            'representation_product_witness': witness_r}


def read_stats(text):
    require(not any(x in text for x in ('FATAL:', 'VERIFY FAILED', 'BAD 4-set', 'too many divisors')),
            'Failure marker in stderr')
    totals = re.findall(r'^TOTAL: (.*)$', text, re.M)
    require(len(totals) == 1, 'Expected one completed TOTAL line')
    stats = {k: int(v) for k, v in re.findall(r'([\w-]+)=(\d+)', totals[0])}
    require(stats['6-sets'] == 0, 'Sextuple reported')
    return stats


def historical_coverage():
    intervals = []; counts = Counter(); repair_quads = 0
    names = ['s4d_1e11.err'] + [f'r12_{k}.err' for k in range(1, 10)]
    names += [f'boundary_{k}.err' for k in range(1, 10)]
    for name in names:
        text = (ROOT/'logs'/name).read_text()
        header = re.search(r's4: S in \[(\d+),\s*(\d+)\), (\d+) threads', text)
        require(header is not None, f'Missing interval header: {name}')
        lo, hi, nt = map(int, header.groups()); stats = read_stats(text)
        require(stats['S_done'] == hi-lo, f'Incomplete interval: {name}')
        rows = re.findall(r'^\s+thread\s+(\d+): S \[(\d+),(\d+)\)', text, re.M)
        require(len(rows) == nt, f'Missing thread completions: {name}')
        for i, row in enumerate(rows):
            require(tuple(map(int, row)) == (i, lo+(hi-lo)*i//nt, lo+(hi-lo)*(i+1)//nt),
                    f'Thread coverage mismatch: {name}')
        intervals.append((lo, hi)); counts.update({k:stats[k] for k in ('5-sets','multi-ext-4sets')})
        if name.startswith('boundary_'):
            require(stats['5-sets'] == stats['multi-ext-4sets'] == 0, 'Boundary output changed')
            require((ROOT/'logs'/name.replace('.err','.out')).read_bytes() == b'', 'Unexpected boundary output')
            repair_quads += stats['4-sets']
    cursor = 1
    for lo, hi in sorted(intervals):
        require(lo == cursor, f'Historical gap or overlap at {cursor}')
        cursor = hi
    require(cursor == 10**12 and repair_quads == 15, 'Historical frontier/repair mismatch')
    return counts


FIVE = re.compile(r'5: \[([\d,]+)\] S=(\d+)')
MULTI = re.compile(r'M: \[([\d,]+)\] ext=\[([\d,]+)\] S=(\d+)')


def scan(stream, lo, hi, five, near):
    """Check every record exactly and derive near misses from each extension pair."""
    counts = Counter(); h = hashlib.sha256(); max_extensions = 0
    for raw in stream:
        h.update(raw); line = raw.decode('ascii').rstrip('\r\n')
        m = FIVE.fullmatch(line)
        if m:
            values = tuple(sorted(map(int, m[1].split(',')))); total = int(m[2])
            require(len(values) == len(set(values)) == 5 and values[0] > 0, 'Invalid five-set')
            require(sum(values[:4]) == total and lo <= total < hi, 'Five-set outside interval')
            require(all(square(a+b) for a,b in combinations(values,2)), 'Nonsquare five-set sum')
            five.add(values); counts['five'] += 1
            continue
        m = MULTI.fullmatch(line)
        require(m is not None, f'Unexpected result line: {line[:150]}')
        total, partial, hits = original.parse_multi(line)
        require(total == int(m[3]) and lo <= total < hi, 'Multi-extension record outside interval')
        require(not hits, 'Positive sextuple in multi-extension record')
        max_extensions = max(max_extensions, len(m[2].split(',')))
        near.update(partial); counts['multi'] += 1
    return counts, h.hexdigest(), max_extensions


def unpack(archive, target):
    """Copy ordinary archive files only, with path containment checks."""
    with tarfile.open(archive, 'r:gz') as tar:
        for item in tar:
            destination = (target/item.name).resolve()
            require(destination.is_relative_to(target.resolve()), 'Unsafe archive path')
            require(item.isfile(), 'Archive must contain ordinary files only')
            destination.parent.mkdir(parents=True, exist_ok=True)
            with tar.extractfile(item) as src, destination.open('xb') as dst:
                shutil.copyfileobj(src, dst)


def check_catalog(saved, near):
    require(set(saved) == set(near), 'Near-miss catalog mismatch')
    for values, bad in near.items():
        record = saved[values]
        require(tuple(record['nonsquare_pair']) == bad and record['square_total'] == square(sum(values)),
                'Near-miss square total or missing edge mismatch')
        require(record['euler_squares'] == original.euler_count(values, bad), 'Euler classification mismatch')


def verify():
    require(__debug__, 'Run without python -O; the archived verifier uses assertions')
    start = time.monotonic()
    for line in (ROOT/'SHA256SUMS').read_text().splitlines():
        expected, name = line.split('  ', 1); path = (ROOT/name).resolve()
        require(path.is_relative_to(ROOT), 'Invalid checksum path')
        require(digest(path) == expected, f'Checksum mismatch: {name}')
    caps = capacity_bounds(); hist_counts = historical_coverage()
    five = set(); baseline = {}
    print('Checking historical raw results and repaired boundaries...', flush=True)
    with gzip.open(ROOT/'evidence/all_1e12.out.gz', 'rb') as src:
        counts, sha, max_ext = scan(src, 1, 10**12, five, baseline)
    require(sha == HISTORICAL_SHA, 'Historical raw-data hash mismatch')
    require(counts['five'] == hist_counts['5-sets'] == 1374180, 'Historical raw five-set count')
    require(counts['multi'] == hist_counts['multi-ext-4sets'] == 3873, 'Historical multi count')
    require(len(five) == 1374167 and len(baseline) == 430, 'Historical unique counts')
    require(max_ext < 64, 'Historical extension truncation cannot be excluded')
    caps['historical_max_extensions_in_complete_raw_output'] = max_ext
    cumulative_five = len(five); del five
    require(sum(square(sum(v)) for v in baseline) == 271, 'Historical square totals')
    old_baseline = dict(baseline); cursor = 10**12; previous = None; batches = []
    with tempfile.TemporaryDirectory(prefix='oeis-a115040-') as temp:
        base = Path(temp)
        for name in RUNS:
            print(f'Checking {name}...', flush=True)
            unpack(ROOT/'evidence'/(name+'.tar.gz'), base)
            directory = base/name
            manifest = json.loads((directory/'manifest.json').read_text())
            audit = json.loads((directory/'audit.json').read_text())
            independent = json.loads((directory/'final-independent-verification.json').read_text())
            status = json.loads((directory/'status.json').read_text())
            require(audit['passed'] and independent['passed'] and status['audit_passed'], 'Recorded audit failed')
            require(manifest['historical_data_sha256'] == HISTORICAL_SHA, 'Historical lineage mismatch')
            predecessor = manifest.get('previous_run')
            if previous is None:
                require(predecessor is None and manifest['start'] == 1001000000000, 'First continuation start')
            else:
                require(predecessor and Path(predecessor['directory']).name == previous.name, 'Wrong predecessor')
                require(predecessor['frontier'] == cursor == manifest['start'], 'Ancestry gap')
                for filename, expected in predecessor['files'].items():
                    require(digest(previous/filename) == expected, 'Predecessor hash mismatch')
            end, files, maxima = original.check_blocks(directory, manifest)
            require(len(manifest['attempts']) == len(manifest['completed_blocks']), 'Incomplete production attempt')
            if previous is None:
                err = (directory/'preflight/frontier.err').read_text()
                require('s4: S in [1000000000000, 1001000000000)' in err, 'Missing frontier bridge')
                stats = read_stats(err); require(stats['S_done'] == 10**9, 'Incomplete bridge')
                files.insert(0, (directory/'preflight/frontier.out', cursor, manifest['start'], stats))
            five = set(); near = {}; total = Counter()
            for path, lo, hi, stats in files:
                require(lo == cursor, f'Coverage gap at {cursor}'); cursor = hi
                with path.open('rb') as src:
                    count, _, _ = scan(src, lo, hi, five, near)
                require(count['five'] == stats['5-sets'] and count['multi'] == stats['multi-ext-4sets'],
                        'Output count mismatch')
                total.update(count)
            require(cursor == end == audit['frontier'] == independent['frontier'] == status['frontier'], 'Frontier mismatch')
            require(len(five) == total['five'] == audit['unique_five_sets'], 'Continuation five-set count')
            require(total['multi'] == audit['raw_counts_including_frontier_preflight']['multi'], 'Multi count')
            # The first independent report predates the positive_sextuples field.
            # Its absence is allowed only there; raw output is checked above.
            if name != RUNS[0]:
                require('positive_sextuples' in independent, 'Missing independent solution list')
            require(not audit['positive_sextuples'] and not independent.get('positive_sextuples', [])
                    and status['six_sets'] == 0, 'Recorded sextuple')
            saved = {tuple(r['values']):r for r in json.loads((directory/'near-misses.json').read_text())}
            check_catalog(saved, near)
            new = set(near)-set(baseline)
            for values, record in saved.items():
                if 'new_vs_prior_frontier' in record:
                    require(record['new_vs_prior_frontier'] == (values not in baseline), 'Novelty mismatch')
                require(record['new_vs_S_lt_1e12'] == (values not in old_baseline), 'Historical novelty mismatch')
            baseline.update(near); cumulative_five += len(five)
            require(len(near) == audit['primitive_near_misses_in_interval']
                    == independent['primitive_near_misses_in_added_interval'], 'Near-miss count mismatch')
            require(len(new) == independent['new_primitive_near_misses'], 'New near-miss count mismatch')
            require(len(baseline) == independent['cumulative_primitive_near_misses'], 'Cumulative near-miss count')
            require(sum(square(sum(v)) for v in baseline) == independent['cumulative_square_total_near_misses'],
                    'Cumulative square-total count')
            batches.append({'run':name, 'frontier':cursor, 'blocks':len(manifest['completed_blocks']),
                            'unique_five_sets':len(five), 'primitive_near_misses':len(near),
                            'new_primitive_near_misses':len(new), 'capacity_high_water':dict(maxima)})
            previous = directory; del five
    require(cursor == FRONTIER and cumulative_five == 3791838 and len(baseline) == 740, 'Final totals')
    require(sum(square(sum(v)) for v in baseline) == 477, 'Cumulative square totals')
    return {'passed':True, 'checked_at':datetime.now(timezone.utc).isoformat(),
            'seconds':round(time.monotonic()-start,3), 'frontier_S_exclusive':cursor,
            'A115040_a6_strict_lower_bound':cursor//4, 'excludes_all_members_at_most_10_pow_12':True,
            'positive_sextuples':0, 'cumulative_unique_five_sets':cumulative_five,
            'cumulative_primitive_near_misses':len(baseline), 'cumulative_square_total_near_misses':477,
            'historical_capacity_check':caps, 'continuations':batches,
            'scope':'Package hashes, contiguous logged coverage including repaired boundaries, '
                    'completion checkpoints, source/binary/output hashes, exact checks of every stored '
                    'five-set and multi-extension record, near-miss classifications, and historical '
                    'capacity bounds. This is an evidence audit, not a fresh enumeration of every quadruple.'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path, help='Write the new summary to this JSON file')
    args = parser.parse_args()
    report = verify(); text = json.dumps(report, indent=2)+'\n'
    if args.report:
        args.report.write_text(text, encoding='utf-8')
    print(text)
