"""Read compact continuation evidence; omitted bulk outputs are explicit.

The caller supplies the portable verifier's exact-arithmetic helpers.
"""
from collections import Counter
from datetime import datetime
import json
from pathlib import Path
import re

RUNS = (
    'hybrid-2026-09-26', 'exhaustive-2026-09-28',
    'exhaustive-to-2e12-2026-09-29', 'second-anchor-2026-10-01',
    'triple-pool-2026-10-02', 'fresh-adaptive-2026-10-03',
    'creative-2026-10-04', 'exhaustive-2026-10-05-8h',
    'exhaustive-2026-10-06-9h', 'exhaustive-2026-10-13-8h',
    'milestone-3e12-2026-10-14-8h',
)
COUNTERS = ('S_done', 'pairs', 'S_ge3', 'triples', '4-sets', '5-sets',
            '6-sets', 'multi-ext-4sets', 'ext_cand', 'ext_sqrt')
ENGINE_SHA = 'dd010875cfdde0148cdc0fc6cccba90f7494ea64d97fc666d7b2d502c3d5ff14'
BINARY_SHA = 'e96ab673b934bbde5704f06189a35c69744aec0e53556bd19e0086aa46e5cb47'


def read(path):
    return json.loads(path.read_text())


def check_checkpoint(v, text, block, manifest):
    lines = text.splitlines()
    lo, hi = block['lo'], block['hi']
    nt = manifest['threads']
    v.require(lines[0] == 's4ckpt v1' and lines[-1] == 'DONE', 'Incomplete checkpoint')
    v.require(tuple(map(int, lines[1].split())) ==
              (lo, hi, nt, 1 << manifest['chunk_log2']), 'Checkpoint parameters')
    v.require(len(lines) == nt+3, 'Checkpoint thread count')
    rows = [list(map(int, line.split())) for line in lines[2:-1]]
    for i, row in enumerate(rows):
        start, end = lo+(hi-lo)*i//nt, lo+(hi-lo)*(i+1)//nt
        v.require(len(row) == 13 and row[:3] == [i, end, end], 'Incomplete thread range')
        v.require(row[3] == end-start and min(row) >= 0, 'Thread coverage count')
    for j, key in enumerate(COUNTERS):
        v.require(sum(row[3+j] for row in rows) == block['totals'][key],
                  'Checkpoint counter mismatch: '+key)
    return rows


def check_predecessor(v, predecessor, previous, name, cursor, manifest):
    old_path = Path(predecessor['directory'])
    old_name = old_path.parent.name if old_path.name == 'exhaustive' else old_path.name
    v.require(old_name == name and predecessor['frontier'] == cursor == manifest['start'],
              'Continuation ancestry gap')
    for filename, expected in predecessor['files'].items():
        path = (previous/filename).resolve()
        v.require(path.is_relative_to(previous.resolve()), 'Invalid predecessor path')
        v.require(v.digest(path) == expected, 'Predecessor hash mismatch: '+filename)


def check_run(v, directory, manifest, raw_root=None):
    audit = read(directory/'audit.json')
    independent = read(directory/'final-independent-verification.json')
    status = read(directory/'status.json')
    recorded = read(directory/'raw-verification.json')
    v.require(audit['passed'] and independent['passed'] and status['audit_passed']
              and recorded['passed'], 'Recorded audit failed')
    v.require(manifest['historical_data_sha256'] == v.HISTORICAL_SHA, 'Historical lineage')
    v.require(manifest['source_sha256']['s4.c'] == ENGINE_SHA
              and manifest['binary_sha256'] == BINARY_SHA, 'Unexpected enumeration engine')
    for name, expected in manifest['source_sha256'].items():
        path = (directory/'sources'/name).resolve()
        v.require(path.is_relative_to((directory/'sources').resolve()), 'Invalid source path')
        v.require(v.digest(path) == expected, 'Source hash mismatch')
    v.require(v.digest(directory/'s4') == BINARY_SHA, 'Binary hash mismatch')
    v.require(recorded['manifest_sha256'] == v.digest(directory/'manifest.json'), 'Raw audit lineage')
    attempts = manifest['attempts']; completed = manifest['completed_blocks']
    v.require(len({b['id'] for b in attempts}) == len(attempts), 'Duplicate attempt')
    v.require([b for b in attempts if b.get('complete')] == completed, 'Attempt/completion mismatch')
    v.require(set(recorded['outputs']) == {b['id'] for b in attempts}, 'Raw audit block list')
    cursor = manifest['start']; near = {}; total = Counter(); maxima = Counter()
    full_five = set(); partials = []
    for block in attempts:
        stem = block['id']; lo, hi = block['lo'], block['hi']
        v.require(re.fullmatch(r'block-\d+', stem) and lo == cursor and hi > lo, 'Coverage gap/order')
        row = recorded['outputs'][stem]
        if not block.get('complete'):
            v.require(block is attempts[-1] and block.get('returncode') != 0, 'Unexpected partial attempt')
            # The sole interrupted interval is preserved, but never advances cursor or census.
            for ext in ('out', 'err', 'ckpt'):
                v.require(v.digest(directory/(stem+'.'+ext)) == row[ext+'_sha256'], 'Partial hash mismatch')
            partial_near = {}; partial_five = set()
            with (directory/(stem+'.out')).open('rb') as stream:
                counts, _, _ = v.scan(stream, lo, hi, partial_five, partial_near)
            v.require(dict(counts) == row['counts'] and len(partial_five) == row['unique_five_sets'],
                      'Partial output count mismatch')
            v.require('DONE' not in (directory/(stem+'.ckpt')).read_text().splitlines(), 'Partial has DONE')
            partials.append({'id':stem, 'lo':lo, 'hi':hi, 'returncode':block['returncode'],
                             'raw_five_sets':counts['five'], 'multi_records':counts['multi'],
                             'contributes_coverage':False})
            continue
        v.require(block['returncode'] == 0, 'Nonzero interval exit')
        for ext, field in (('err', 'stderr_sha256'), ('ckpt', 'checkpoint_sha256')):
            v.require(v.digest(directory/(stem+'.'+ext)) == block[field], 'Completion file hash mismatch')
        rows = check_checkpoint(v, (directory/(stem+'.ckpt')).read_text(), block, manifest)
        err = (directory/(stem+'.err')).read_text()
        stats = v.read_stats(err)
        v.require(stats == block['totals'] and stats['S_done'] == hi-lo, 'TOTAL counters mismatch')
        header = f's4: S in [{lo}, {hi}), {manifest["threads"]} threads, chunk {1 << manifest["chunk_log2"]}, ext=on, checkpointing'
        v.require(header in err, 'Wrong worker interval or disabled extensions')
        threads = re.findall(r'thread\s+(\d+): S \[(\d+),(\d+)\) 4-sets (\d+) 5-sets (\d+)', err)
        expected_threads = [(str(i), str(lo+(hi-lo)*i//len(rows)), str(row[2]), str(row[7]), str(row[8]))
                            for i, row in enumerate(rows)]
        v.require(threads == expected_threads, 'Thread log/checkpoint mismatch')
        cap = re.search(r'CAPACITY: reps=(\d+)/(\d+) divisors=(\d+)/(\d+) extensions=(\d+)/(\d+)', err)
        v.require(cap is not None, 'Missing capacity record')
        for i, key in enumerate(('representations', 'divisors', 'extensions')):
            used, limit = int(cap[2*i+1]), int(cap[2*i+2])
            v.require(used <= limit and block['capacity_maxima'][key] == {'used':used, 'limit':limit},
                      'Capacity mismatch')
            maxima[key] = max(maxima[key], used)
        excerpt = directory/(stem+'.multi.out')
        v.require(v.digest(excerpt) == row['multi_sha256'], 'Multi-extension excerpt hash')
        with excerpt.open('rb') as stream:
            counts, _, _ = v.scan(stream, lo, hi, set(), near)
        v.require(counts['five'] == 0 and counts['multi'] == stats['multi-ext-4sets'], 'Excerpt count mismatch')
        v.require(row['out_sha256'] == block['output_sha256'], 'Omitted-output hash commitment mismatch')
        v.require(row['counts'].get('five', 0) == stats['5-sets']
                  and row['counts'].get('multi', 0) == stats['multi-ext-4sets']
                  and row['unique_five_sets'] == stats['5-sets'], 'Recorded raw count mismatch')
        if raw_root is not None:
            raw = raw_root/directory.name/'exhaustive'/(stem+'.out')
            with raw.open('rb') as stream:
                full_counts, sha, _ = v.scan(stream, lo, hi, full_five, {})
            v.require(sha == block['output_sha256'] and dict(full_counts) == row['counts'], 'Full raw mismatch')
            with raw.open('rb') as stream:
                v.require(b''.join(line for line in stream if line.startswith(b'M:')) == excerpt.read_bytes(),
                          'Excerpt differs from full raw output')
        total.update({'five':stats['5-sets'], 'multi':stats['multi-ext-4sets']})
        cursor = hi
    v.require(cursor == audit['frontier'] == independent['frontier'] == status['frontier'], 'Frontier mismatch')
    v.require(len(completed) == audit['completed_blocks'] == independent['completed_blocks']
              == status['completed_blocks'], 'Completed block count')
    v.require(len(attempts) == audit['attempted_blocks'] == independent['attempted_blocks'], 'Attempt count')
    v.require(total['five'] == audit['unique_five_sets'] == independent['unique_five_sets_in_added_interval']
              == status['five_sets'] == recorded['completed_unique_five_sets'], 'Five-set count')
    v.require(total['five'] == audit['raw_counts_including_frontier_preflight']['five']
              and total['multi'] == audit['raw_counts_including_frontier_preflight']['multi']
              == status['multi_lines'], 'Raw census counters')
    if raw_root is not None:
        v.require(len(full_five) == total['five'], 'Full raw duplicate five-set')
    v.require(not audit['positive_sextuples'] and not independent['positive_sextuples']
              and status['six_sets'] == 0, 'Recorded sextuple')
    v.require(dict(maxima) == audit['capacity_high_water'] == independent['capacity_high_water'], 'Capacity totals')
    return cursor, total['five'], near, independent, maxima, partials


def check_certificate(v, directory, frontier):
    folder = directory/'certification'; certificate = read(folder/'milestone.json')
    v.require(certificate['passed'] and certificate['both_exhaustive_audits_passed']
              and certificate['frontier'] == frontier
              and certificate['member_strict_lower_bound'] == frontier//4, 'Milestone certificate')
    v.require(v.digest(directory/'manifest.json') == certificate['exhaustive_manifest_sha256'], 'Certified manifest')
    for name, sha in certificate['audit_copies'].items():
        v.require(v.digest(folder/name) == sha, 'Milestone audit hash')
        audit = read(folder/name)
        v.require(audit['passed'] and audit['frontier'] == frontier and not audit['positive_sextuples'], 'Milestone audit')
    certified = datetime.fromisoformat(certificate['certified_at'])
    for name in ('milestone-audit-exit.json', 'milestone-independent-exit.json'):
        record = read(folder/name)
        v.require(record['exit_code'] == 0 and not record['deadline_reached']
                  and datetime.fromisoformat(record['ended_at']) <= certified, 'Milestone verifier exit')
    return certificate['certified_at']
