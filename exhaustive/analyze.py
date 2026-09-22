#!/usr/bin/env python3
"""Exactly summarize an s4 output file (.gz supported), with extension pairs split.

Usage: python3 -B analyze.py evidence/all_1e12.out.gz 1000000000000
"""
import gzip
import json
from pathlib import Path
import sys
from decimal import Decimal
from verify import scan, square


def main():
    if len(sys.argv) != 3:
        raise SystemExit(__doc__)
    path = Path(sys.argv[1]); upper = int(Decimal(sys.argv[2]))
    five = set(); near = {}
    opener = gzip.open if path.suffix == '.gz' else open
    with opener(path, 'rb') as src:
        counts, sha, maximum = scan(src, 1, upper, five, near)
    result = {'scope': 'Exact checks of this result file; coverage requires separate logs and the verifier.',
              'S_exclusive': upper, 'raw_sha256': sha, 'five_set_lines': counts['five'],
              'unique_five_sets': len(five), 'duplicate_five_lines': counts['five']-len(five),
              'multi_extension_lines': counts['multi'], 'maximum_extensions': maximum,
              'primitive_six_member_near_misses': len(near),
              'near_misses_with_square_total': sum(square(sum(v)) for v in near),
              'positive_sextuples': 0,
              'primitive_near_misses': [dict(values=v,nonsquare_pair=near[v],square_total=square(sum(v)))
                                      for v in sorted(near)]}
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
