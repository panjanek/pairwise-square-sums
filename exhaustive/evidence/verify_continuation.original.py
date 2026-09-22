"""Independent post-run check. Reads predecessors; writes only the selected run.

Usage: python3 exhaustive/verify_continuation.py runs/exhaustive-2026-09-18
No search, controller, classifier, or first-auditor modules are imported.
"""
import ast
import hashlib
import itertools
import json
import math
import re
import sys
from collections import Counter
from datetime import datetime, timezone
from functools import lru_cache, reduce
from pathlib import Path


def digest(path):
    h = hashlib.sha256()
    with path.open('rb') as f:
        for part in iter(lambda: f.read(1024*1024), b''):
            h.update(part)
    return h.hexdigest()


def square(n):
    return n >= 0 and math.isqrt(n)**2 == n


@lru_cache(None)
def square_factor(n):
    result = 1
    d = 2
    while d*d <= n:
        while n % (d*d) == 0:
            n //= d*d; result *= d*d
        d += 1
    return result


def parse_multi(line):
    four, extensions = (ast.literal_eval(g) for g in re.findall(r'\[[^]]*\]', line))
    assert len(four) == 4 and len(extensions) >= 2
    assert len(set(four+extensions)) == len(four+extensions) and min(four+extensions)>0
    assert all(square(a+b) for a, b in itertools.combinations(four, 2))
    assert all(square(a+b) for a in four for b in extensions)
    near = {}; solutions = set()
    for a, b in itertools.combinations(extensions, 2):
        vals = four+[a, b]
        if square(a+b):
            solutions.add(tuple(sorted(vals)))
        else:
            k = square_factor(reduce(math.gcd, vals))
            primitive = tuple(sorted(v//k for v in vals))
            assert sum(square(x+y) for x, y in itertools.combinations(primitive, 2)) == 14
            near[primitive] = tuple(sorted((a//k, b//k)))
    return sum(four), near, solutions


def euler_count(vals, bad):
    count = 0
    for third in vals:
        if third in bad:
            continue
        x = bad+(third,); y = tuple(v for v in vals if v not in x)
        r = [[math.isqrt(x[(i+j)%3]+y[(i+2*j)%3]) for j in range(3)] for i in range(3)]
        for signs in itertools.product((-1, 1), repeat=6):
            rows = [r[0], [r[1][j]*signs[j] for j in range(3)], [r[2][j]*signs[j+3] for j in range(3)]]
            if all(sum(a*b for a, b in zip(rows[i], rows[j])) == 0
                   for i, j in itertools.combinations(range(3), 2)):
                count += 1
                break
    return count


def check_blocks(directory, manifest):
    for name, sha in manifest['source_sha256'].items():
        assert digest(directory/'sources'/name) == sha
    assert digest(directory/'s4') == manifest['binary_sha256']
    cursor = manifest['start']; files = []; maxima = Counter()
    for b in manifest['completed_blocks']:
        assert b['lo'] == cursor and b['complete'] and b['returncode'] == 0
        cursor = b['hi']
        for ext, field in [('out', 'output_sha256'), ('err', 'stderr_sha256'), ('ckpt', 'checkpoint_sha256')]:
            assert digest(directory/(b['id']+'.'+ext)) == b[field]
        cp = (directory/(b['id']+'.ckpt')).read_text().splitlines()
        assert cp[0] == 's4ckpt v1' and cp[-1] == 'DONE'
        lo, hi, nt, width = map(int, cp[1].split())
        assert (lo, hi, nt, width) == (b['lo'], b['hi'], manifest['threads'], 1<<manifest['chunk_log2'])
        assert len(cp) == nt+3
        for i, line in enumerate(cp[2:-1]):
            row = list(map(int, line.split()))
            assert row[0] == i and row[1] == row[2] == lo+(hi-lo)*(i+1)//nt
        err = (directory/(b['id']+'.err')).read_text()
        assert not any(s in err for s in ('FATAL:', 'VERIFY FAILED', 'BAD 4-set'))
        total = re.findall(r'^TOTAL: (.*)$', err, re.M)
        assert len(total) == 1
        stats = {k: int(v) for k, v in re.findall(r'([\w-]+)=(\d+)', total[0])}
        assert stats == b['totals'] and stats['S_done'] == hi-lo
        cap = re.search(r'CAPACITY: reps=(\d+)/(\d+) divisors=(\d+)/(\d+) extensions=(\d+)/(\d+)', err)
        assert cap
        for i, key in enumerate(('representations', 'divisors', 'extensions')):
            used, limit = int(cap[2*i+1]), int(cap[2*i+2])
            assert used <= limit and b['capacity_maxima'][key] == {'used': used, 'limit': limit}
            maxima[key] = max(maxima[key], used)
        files.append((directory/(b['id']+'.out'), lo, hi, stats))
    return cursor, files, maxima


def main(directory):
    m = json.loads((directory/'manifest.json').read_text())
    audit = json.loads((directory/'audit.json').read_text())
    status = json.loads((directory/'status.json').read_text())
    assert audit['passed'] and status['audit_passed'] and status['status'] in ('budget_exhausted', 'completed', 'solution_found')
    frontier, files, maxima = check_blocks(directory, m)
    assert frontier == audit['frontier'] == status['frontier']
    historical = Path.home()/'ps/all_1e12.out'
    assert digest(historical) == m['historical_data_sha256']
    baseline = {}
    for line in historical.open():
        if line.startswith('M:'):
            _, near, hits = parse_multi(line); assert not hits
            baseline.update(near)
    assert len(baseline) == 430
    original = dict(baseline)
    cumulative_five_before = 1374167
    prior = m.get('previous_run'); expected = m['start']; visited = set()
    while prior:
        path = Path(prior['directory'])
        assert path not in visited; visited.add(path)
        for name, sha in prior['files'].items():
            assert digest(path/name) == sha
        pm = json.loads((path/'manifest.json').read_text())
        pa = json.loads((path/'audit.json').read_text())
        independent = json.loads((path/'final-independent-verification.json').read_text())
        assert pa['passed'] and independent['passed'] and not pa['positive_sextuples']
        end, pf, _ = check_blocks(path, pm)
        assert end == expected == prior['frontier'] == pa['frontier']
        if not pm.get('previous_run'):
            pf.insert(0, (path/'preflight/frontier.out', 10**12, pm['start'], None))
        count = 0
        for source, lo, hi, _ in pf:
            for line in source.open():
                if line.startswith('M:'):
                    total, near, hits = parse_multi(line)
                    assert lo <= total < hi and not hits
                    baseline.update(near)
                elif line.startswith('5:'):
                    count += 1
        assert count == pa['raw_counts_including_frontier_preflight']['five']
        cumulative_five_before += pa['unique_five_sets']
        expected = pm['start']; prior = pm.get('previous_run')
    assert expected == 1001000000000
    five = set(); near = {}; solutions = set(); counts = Counter()
    for source, lo, hi, stats in files:
        local = Counter()
        for line in source.read_text().splitlines():
            s = int(line.rsplit('S=', 1)[1]); assert lo <= s < hi
            if line.startswith('5:'):
                vals = tuple(sorted(ast.literal_eval(line[3:line.index(']')+1])))
                assert len(vals) == len(set(vals)) == 5 and min(vals)>0 and sum(vals[:4]) == s
                assert all(square(a+b) for a, b in itertools.combinations(vals, 2))
                five.add(vals); local['five'] += 1
            elif line.startswith('M:'):
                total, part, hits = parse_multi(line); assert total == s
                near.update(part); solutions.update(hits); local['multi'] += 1
            else:
                assert line.startswith('!!!!!!!! FOUND 6-SET:')
                vals = tuple(sorted(ast.literal_eval(re.search(r'\[[^]]*\]', line)[0])))
                assert len(vals) == len(set(vals)) == 6 and min(vals)>0
                assert all(square(a+b) for a, b in itertools.combinations(vals, 2))
                solutions.add(vals); local['six'] += 1
        assert (local['five'], local['multi'], local['six']) == (stats['5-sets'], stats['multi-ext-4sets'], stats['6-sets'])
        counts.update(local)
    alerts = directory/'SIXSET_FOUND.txt'
    if alerts.exists():
        for line in alerts.read_text().splitlines():
            assert line.startswith('!!!!!!!! FOUND 6-SET:')
            vals = tuple(sorted(ast.literal_eval(re.search(r'\[[^]]*\]', line)[0])))
            assert len(vals) == len(set(vals)) == 6 and min(vals)>0
            assert all(square(a+b) for a, b in itertools.combinations(vals, 2))
            solutions.add(vals)
    saved = {tuple(r['values']): r for r in json.loads((directory/'near-misses.json').read_text())}
    assert saved.keys() == near.keys()
    hist = Counter()
    for vals, bad in near.items():
        r = saved[vals]
        assert tuple(r['nonsquare_pair']) == bad and r['square_total'] == square(sum(vals))
        assert r['new_vs_prior_frontier'] == (vals not in baseline)
        assert r['new_vs_S_lt_1e12'] == (vals not in original)
        count = euler_count(vals, bad); assert r['euler_squares'] == count
        if vals not in baseline:
            hist[count] += 1
    new = {v: b for v, b in near.items() if v not in baseline}
    cumulative = dict(baseline); cumulative.update(near)
    assert len(baseline) == audit['baseline_primitive_near_misses']
    assert len(five) == audit['unique_five_sets'] and dict(counts) == {k: v for k, v in audit['raw_counts_including_frontier_preflight'].items() if v}
    assert len(near) == audit['primitive_near_misses_in_interval'] and len(new) == audit['new_primitive_near_misses']
    assert {str(k): v for k, v in hist.items()} == audit['new_near_miss_euler_histogram']
    assert sum(square(sum(v)) for v in new) == audit['new_square_total_near_misses']
    assert len(cumulative) == audit['cumulative_primitive_near_misses']
    assert cumulative_five_before+len(five) == audit['cumulative_unique_five_sets']
    assert sum(square(sum(v)) for v in cumulative) == audit['cumulative_square_total_near_misses']
    assert sorted(solutions) == [tuple(v) for v in audit['positive_sextuples']]
    parse = datetime.fromisoformat
    result = {'passed': True, 'checked_at': datetime.now(timezone.utc).isoformat(),
              'verification_script_sha256': digest(Path(__file__)), 'frontier': frontier,
              'third_largest_strict_lower_bound': frontier//4,
              'bound_improvement_percent_vs_previous': (frontier/m['start']-1)*100,
              'completed_blocks': len(m['completed_blocks']), 'attempted_blocks': len(m['attempts']),
              'baseline_primitive_near_misses': len(baseline), 'unique_five_sets_in_added_interval': len(five),
              'cumulative_unique_five_sets': cumulative_five_before+len(five),
              'primitive_near_misses_in_added_interval': len(near), 'new_primitive_near_misses': len(new),
              'new_square_total_near_misses': sum(square(sum(v)) for v in new),
              'cumulative_primitive_near_misses': len(cumulative),
              'cumulative_square_total_near_misses': sum(square(sum(v)) for v in cumulative),
              'new_euler_square_histogram': dict(hist), 'positive_sextuples': sorted(solutions),
              'capacity_high_water': dict(maxima),
              'search_wall_seconds': (parse(m['completed_blocks'][-1]['ended_at'])-parse(m['launched_at'])).total_seconds(),
              'summed_worker_seconds': sum(b['elapsed_seconds'] for b in m['completed_blocks']),
              'budget_elapsed_through_automatic_audit_seconds': (parse(status['finished_at'])-parse(m['budget_started_at'])).total_seconds(),
              'total_quadruples': sum(b['totals']['4-sets'] for b in m['completed_blocks']),
              'smallest_new_near_misses_by_largest_member': sorted(new, key=lambda v: v[-1])[:5]}
    (directory/'final-independent-verification.json').write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main(Path(sys.argv[1]).resolve())
