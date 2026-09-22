#!/usr/bin/env python3
"""Seal public files; private submission notes and generated reports are excluded."""
import hashlib
from pathlib import Path

ROOT = Path(__file__).resolve().parent
TOP = ('.gitattributes', '.gitignore', 'LICENSE', 'README.md',
       's4.c', 'analyze.py', 'analysis_1e12.txt',
       'verify.py', 'test_verify.py', 'make_checksums.py')


def main():
    paths = [ROOT/name for name in TOP]
    paths += [p for folder in ('logs', 'evidence') for p in (ROOT/folder).rglob('*')
              if p.is_file() and '__pycache__' not in p.parts]
    records = []
    for path in sorted(paths):
        h = hashlib.sha256()
        with path.open('rb') as f:
            for part in iter(lambda: f.read(1024*1024), b''):
                h.update(part)
        records.append(h.hexdigest()+'  '+path.relative_to(ROOT).as_posix())
    (ROOT/'SHA256SUMS').write_text('\n'.join(records)+'\n', encoding='ascii', newline='\n')
    print(f'Wrote SHA256SUMS for {len(records)} files. Run verify.py next.')


if __name__ == '__main__':
    main()
