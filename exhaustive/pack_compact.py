"""Build compact evidence from existing runs, checking full raw outputs first.

No enumeration is performed. Requires the original run directories locally.
Usage: python3 -B pack_compact.py /path/to/runs
The eleven *-compact.tar.gz files are written under evidence/.
"""
import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import shutil
import tarfile
import tempfile
import compact
import verify as v


def build(runs):
    v.require(__debug__, 'Run without -O')
    for name in compact.RUNS:
        print('Checking full raw data and packing '+name, flush=True)
        source = runs/name/'exhaustive'
        manifest = compact.read(source/'manifest.json')
        with tempfile.TemporaryDirectory(prefix='oeis-compact-') as tmp:
            target = Path(tmp)/name; target.mkdir()
            for filename in ('manifest.json', 'audit.json', 'final-independent-verification.json',
                             'status.json', 'near-misses.json', 's4'):
                shutil.copyfile(source/filename, target/filename)
            for filename in manifest['source_sha256']:
                dest = target/'sources'/filename; dest.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(source/'sources'/filename, dest)
            outputs = {}; five = set()
            for block in manifest['attempts']:
                stem = block['id']; row = {}; local = set()
                for ext in ('err', 'ckpt'):
                    shutil.copyfile(source/(stem+'.'+ext), target/(stem+'.'+ext))
                    row[ext+'_sha256'] = v.digest(target/(stem+'.'+ext))
                with (source/(stem+'.out')).open('rb') as stream:
                    count, sha, _ = v.scan(stream, block['lo'], block['hi'], local, {})
                row.update(out_sha256=sha, counts=dict(count), unique_five_sets=len(local))
                if block.get('complete'):
                    v.require(sha == block['output_sha256'], 'Original output hash mismatch')
                    v.require(count['five'] == block['totals']['5-sets']
                              and count['multi'] == block['totals']['multi-ext-4sets'], 'Raw count mismatch')
                    five.update(local)
                    with (source/(stem+'.out')).open('rb') as stream:
                        excerpt = b''.join(line for line in stream if line.startswith(b'M:'))
                    (target/(stem+'.multi.out')).write_bytes(excerpt)
                    row['multi_sha256'] = hashlib.sha256(excerpt).hexdigest()
                else:
                    shutil.copyfile(source/(stem+'.out'), target/(stem+'.out'))
                outputs[stem] = row
            audit = compact.read(source/'audit.json')
            v.require(len(five) == audit['unique_five_sets'], 'Full raw unique five-set count')
            report = {'passed':True, 'checked_at':datetime.now(timezone.utc).isoformat(),
                      'manifest_sha256':v.digest(source/'manifest.json'),
                      'builder_sha256':v.digest(Path(__file__)), 'verifier_sha256':v.digest(v.ROOT/'verify.py'),
                      'completed_unique_five_sets':len(five), 'outputs':outputs,
                      'scope':'Full original outputs read locally: SHA-256, exact positive distinct five-sets '
                              'and ten square sums, every multi-extension pair, no sextuples. '
                              'Completed bulk five-set outputs omitted from the compact archive. '
                              'The portable default audit checks this record, not those omitted bytes.'}
            (target/'raw-verification.json').write_text(json.dumps(report, indent=2)+'\n')
            if name == compact.RUNS[-1]:
                cert = target/'certification'; cert.mkdir()
                for filename in ('milestone.json', 'milestone-audit.json',
                                 'milestone-final-independent-verification.json',
                                 'milestone-audit-exit.json', 'milestone-independent-exit.json'):
                    shutil.copyfile(source.parent/filename, cert/filename)
            compact.check_run(v, target, manifest)
            archive = v.ROOT/'evidence'/(name+'-compact.tar.gz')
            with tarfile.open(archive, 'w:gz', compresslevel=9) as tar:
                for path in sorted(target.rglob('*')):
                    if path.is_file():
                        info = tar.gettarinfo(str(path), str(path.relative_to(Path(tmp))))
                        info.uid = info.gid = info.mtime = 0; info.uname = info.gname = ''
                        with path.open('rb') as stream:
                            tar.addfile(info, stream)
            print(f'  {len(five):,} distinct five-sets checked; archive {archive.stat().st_size:,} bytes', flush=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('runs', type=Path)
    build(parser.parse_args().runs)
