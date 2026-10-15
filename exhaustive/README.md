# A115040: a computational lower bound for positive square-sum sextuples

**Result, certified 14 October 2026 (UTC):**

> **a(6) > 3 * 10^12.**

Here a(6) is the least possible largest member of a set of six **distinct positive integers** whose 15 pairwise sums are perfect squares, if such a set exists. This is a computational exclusion, not a determination of a(6) or a proof that no positive sextuple exists at any size.

This package supports the updated lower bound for [OEIS A115040](https://oeis.org/A115040). It extends the earlier submission's evidence from S < 4,763,200,000,000 to S < 12,000,000,000,000, using the same enumeration source. The new continuations are supplied as compact evidence archives; their bulk five-set outputs remain outside this repository.

## 1. What was searched, and why it proves the bound

Write a candidate sextuple in increasing order as

    0 < a < b < c < d < e < f,    S = a+b+c+d.

The completed census excludes every sextuple with **S < 12,000,000,000,000**. The two largest members e and f are not independently capped by the search.

Since a, b, c are all smaller than d, we have S < 4d. Thus d <= 3,000,000,000,000 would imply S < 12,000,000,000,000, contradicting the exhaustive exclusion. Consequently d, and therefore f, must exceed **3,000,000,000,000**.

In particular, **there is no sextuple whose six members are all at most 3 * 10^12**. This includes the endpoint 3 * 10^12. The distinction between S (the sum of four members) and f (the largest member) is essential: the older census to S < 10^12 alone gave only the bound 2.5*10^11.

### Coverage ledger

All intervals below are half-open: the left endpoint is included, the right excluded.

| Evidence | Added S interval | Completed production blocks | Unique five-sets added |
|---|---|---:|---:|
| Historical census, with nine singleton repairs | [1, 1,000,000,000,000) | 10 original slices + 9 repairs | 1,374,167 |
| 16-17 September continuation, including its preflight bridge | [1,000,000,000,000, 1,954,600,000,000) | bridge + 192 | 759,709 |
| 18-19 September continuation | [1,954,600,000,000, 2,723,100,000,000) | 155 | 512,820 |
| 19-20 September continuation | [2,723,100,000,000, 3,512,100,000,000) | 159 | 471,454 |
| 20-21 September continuation | [3,512,100,000,000, 4,763,200,000,000) | 252 | 673,688 |
| 26-28 September (`hybrid-2026-09-26`) | [4,763,200,000,000, 7,120,600,000,000) | 474 | 1,105,179 |
| 28-29 September | [7,120,600,000,000, 7,800,700,000,000) | 139 | 291,704 |
| 29 September, to a(6) > 2 * 10^12 | [7,800,700,000,000, 8,000,000,000,000) | 41 | 83,640 |
| 1 October (`second-anchor`) | [8,000,000,000,000, 8,489,300,000,000) | 100 | 201,745 |
| 2 October (`triple-pool`) | [8,489,300,000,000, 9,070,200,000,000) | 119 | 233,721 |
| 3 October (`fresh-adaptive`) | [9,070,200,000,000, 9,698,000,000,000) | 129 | 246,550 |
| 4 October (`creative`) | [9,698,000,000,000, 10,047,600,000,000) | 72 | 134,238 |
| 5 October | [10,047,600,000,000, 10,596,500,000,000) | 113 | 207,410 |
| 6 October | [10,596,500,000,000, 11,208,800,000,000) | 125 | 227,044 |
| 13 October | [11,208,800,000,000, 11,762,800,000,000) | 114 | 200,219 |
| 14 October, milestone | [11,762,800,000,000, 12,000,000,000,000) | 49 | 84,566 |
| **Total** | **[1, 12,000,000,000,000)** | **2,233 continuation blocks + bridge + historical coverage** | **6,807,854** |

No sextuple was found. Every interval counted above completed with exit code zero, and both recorded exhaustive audits passed for each continuation. One interrupted interval in `hybrid-2026-09-26`, [7,120,600,000,000, 7,121,000,000,000), is preserved but contributes no coverage or census counts; the next run starts at its left endpoint and covers it completely.

The final 49 intervals comprise one interval of 100 million S, 47 of five billion, and one of 2.1 billion. The milestone certificate was issued at **2026-10-14 23:00:32.703309 UTC** after both exhaustive verifiers exited successfully. It is included with its hashed audit copies. Later heuristic experiments are outside this evidence package and contribute nothing to the bound.

The cumulative auxiliary catalog contains 983 primitive six-member near misses with exactly 14 square pair sums, including 628 whose total is square. Primitive means that no common square factor greater than one can be removed; it does not mean that the gcd is one. These counts are not a probability model for finding a sextuple.

## 2. Verify the supplied evidence (no search)

Requires **Python 3.10 or later**, standard library only. From this folder:

```sh
python3 -B verify.py --report verification.local.json
```

On Windows, `python` may replace `python3`. Do not use `-O`, which disables assertions used by the archived checker. The verifier uses one process, reads the compressed evidence, temporarily expands the historical raw records and the compact continuation evidence, and removes that temporary copy afterwards. It does not execute the archived binaries or start an exhaustive run. Allow a few minutes and roughly 1 GB of free memory. All paths used for verification are relative to this folder; there is no dependency on the author's WSL home directory or another repository.

The verifier checks:

- SHA-256 hashes from `SHA256SUMS`, then original source, binary, checkpoint, and predecessor hashes inside the manifests. Included raw outputs are hashed directly; omitted bulk outputs have recorded hash commitments and full-raw audit records.
- Contiguous coverage from S = 1, including all nine repaired historical boundaries and the bridge [10^12, 10^12+10^9).
- Completed per-thread ranges, counters, `DONE` checkpoints, failure markers, and continuation capacity maxima.
- Positivity, distinctness, the reported S, and all ten pair sums of every stored five-set using exact integer square roots.
- Every stored multi-extension quadruple, all of its extensions, and every extension pair; any square remaining pair would be a sextuple and fails the claimed exclusion.
- Primitive near-miss catalogs, square totals, Euler classifications, novelty against preceding catalogs, historical capacity bounds, and the final milestone certificate.

[verification.json](verification.json) records a successful verification of this package. It is a generated report, excluded from its own checksum list. `SHA256SUMS` preserves exact bytes of the supporting files; `.gitattributes` prevents line-ending conversion during a Git checkout. A locally generated `verification.local.json` is also excluded from the published checksum list.

### Compact continuation evidence and optional raw-data check

The four original continuation archives and historical raw file are unchanged. The eleven new `*-compact.tar.gz` archives contain the original manifests, both exhaustive audit reports, status, near-miss catalogs, source snapshots and binary, every completed interval's stderr and checkpoint, and **every `M:` multi-extension record**. The unfinished interval's small raw output is also included. No heuristic output is included.

To keep this update small, the new archives omit completed intervals' bulk `5:` output lines. Before packaging, `pack_compact.py` read those full files locally, matched their original SHA-256 hashes and counters, checked all ten sums of every five-set, deduplicated the results, and checked every extension pair. Each archive includes the resulting `raw-verification.json`. The default portable verifier checks those recorded five-set counts and hash commitments against the original completion evidence; **it cannot recompute a hash or check five-set members from bytes that are not supplied**. All multi-extension records are included and rechecked directly.

If the original run tree is available locally, the same verifier can additionally recheck the omitted raw files and compare the excerpts byte for byte:

```sh
python3 -B verify.py --raw-root /path/to/original/runs --report verification.local.json
```

It expects `runs/<run-name>/exhaustive/block-*.out`. This option is unnecessary for auditing the supplied compact evidence; a fresh enumeration from `s4.c` is another way to reproduce the search. `pack_compact.py /path/to/original/runs` rebuilds the eleven compact archives after checking the full outputs, without running a search. Rebuilding changes audit timestamps and requires regenerating checksums.

### Scope of the computational evidence

This is an audit of the supplied census records, compact continuation evidence, and recorded full-raw checks, supported by the exhaustive algorithm and its tests. It is **not** a new implementation independently enumerating the entire historical range, and it is not a formal proof-assistant certificate. Checking reported solutions alone cannot prove that an enumerator omitted none; the completeness argument, implementation, coverage logs, and validation checks together support the computational bound. An editor can reproduce the enumeration using the supplied C source.

## 3. Historical repairs and implementation provenance

The original September 5 slicing script skipped S = k*10^11 for k = 1,...,9. Those nine singleton intervals were subsequently enumerated on September 15. Their logs and empty result files are included in `logs/boundary_*.err` and `.out`: 15 quadruples, no five-sets, no sextuples. The original logs are preserved as written; their gaps are explicitly filled, not silently edited away.

The historical raw file contains 1,374,180 five-set lines, including 13 duplicates, hence 1,374,167 distinct five-sets. `evidence/analysis_1e12.legacy.txt` is preserved as a historical artifact only: its old analyzer treated an entire multi-extension line as one configuration and reported 427 objects. Enumerating every pair of extensions correctly gives **430 primitive six-member near misses**, including **271 with square total**. The corrected `analyze.py` and current `analysis_1e12.txt` use that convention and make no discovery-probability claims.

The historical source snapshot is `evidence/s4-historical.c`. It had fixed-capacity arrays with silent truncation paths. The packaged verifier closes this specific capacity concern for S < 10^12:

- For any difference D < 10^12, the maximum divisor count is **6720**, below the array capacity 16384.
- For S < 10^12, the number of positive unordered representations as two squares is at most **192**, below 512.
- The complete historical raw output has at most **3 extensions per quadruple**. Reaching the old 64-extension limit would have emitted a 64-entry `M:` record, so the checked raw output excludes that truncation case.

The first two bounds are recomputed by finite enumeration of nonincreasing prime-exponent patterns. Moving larger exponents to smaller primes reduces the integer without changing its divisor count; for representations, only primes congruent to 1 modulo 4 contribute, with at most ceil(product(exponent+1)/2) positive unordered representations. See `capacity_bounds()` in `verify.py`.

The current `s4.c` is the exact source used by all fifteen continuations. It aborts on representation, divisor, extension, and bucket overflows instead of silently truncating; records capacity high-water marks; and checks result writes and thread creation. It passed the recorded small-census equivalence, deliberately forced capacity failures, interrupted checkpoint/resume equivalence, and frontier-interval checks. The first continuation archive contains `preflight/summary.json` and the bridge output. Across the completed continuations the largest observed array usages were 288/512 representations, 6480/16384 divisors, and 3/64 extensions.

Source hashes:

```text
historical s4.c  0f06803562bdacd050496258cc2ffd8eda12fbe2a0928ed9a0367f7307cddd71
current s4.c     dd010875cfdde0148cdc0fc6cccba90f7494ea64d97fc666d7b2d502c3d5ff14
historical raw  1dca944413520b5cf1596c4c5820b454e1037c13177ff479d39435e5ee4d5301
```

The run manifests are preserved byte-for-byte, including original command paths, so their ancestry hashes remain valid. The portable verifier maps predecessor names to the supplied archives. Those old command paths are provenance, not paths a reader needs to recreate.

## 4. Algorithm and completeness argument

### Enumerate every square-sum quadruple

For a quadruple Q = {a,b,c,d}, the three pairings give

    S = (a+b)+(c+d) = (a+c)+(b+d) = (a+d)+(b+c).

Thus S has three distinct unordered representations as a sum of two positive squares. Conversely, choose three representations S = x_i+(S-x_i), with both summands square, and set

    a = ( x1 + x2 + x3 - S)/2
    b = ( x1 - x2 - x3 + S)/2
    c = (-x1 + x2 - x3 + S)/2
    d = (-x1 - x2 + x3 + S)/2.

The six pair sums are the six chosen squares. Swapping members of the three representations gives eight choices, which reduce under permutation to two candidate quadruples. The program enumerates both, checking integrality, positivity, and distinctness. If a representation has equal summands, the two classes coincide and are handled once. The modular exclusions preserve all positive integer quadruples.

`process_chunk` buckets every representation p^2+q^2 with 1 <= p <= q in the requested S interval, then examines every triple of representations. Nonprimitive quadruples are retained: they can have primitive extensions.

### Enumerate every positive integer extension

Choose two distinct members A > B of Q. Any common extension e satisfies

    e+A = u^2, e+B = w^2, so (u-w)(u+w) = A-B = D.

Every extension therefore comes from a positive divisor pair D = f*g with f < g and equal parity:

    u = (f+g)/2, w = (g-f)/2, e = u^2-A.

All such pairs are tested; e must be positive, distinct from Q, and have square sums with the other two members. The factorization of D is obtained from known pair roots using A-B = r_AC^2-r_BC^2. Congruence filters reject only nonsquares; surviving tests use exact 128-bit arithmetic.

For the full extension list E, every pair e,f in E is checked for square e+f. Any positive sextuple occurs when Q is its four smallest members, so enumerating all Q below the S frontier and all their extensions is sufficient. Each five-set is reported only from its four smallest members. A multi-extension `M:` line can represent several near misses; its extension pairs are considered separately.

## 5. Build and reproduce

On Linux or WSL Ubuntu, using GCC with 128-bit integers:

```sh
gcc -O3 -march=native -Wall -Wextra -o s4 s4.c -lm -lpthread
# Small validation run; --verify also checks every generated quadruple.
./s4 1 10000000 4 20 --verify --out small.out --ckpt small.ckpt
# Expected: 123 five-sets, 2 multi-extension quadruples, 0 sextuples.
```

`-march=native` is optional. The recorded continuations used GCC 13.3 on Ubuntu 24.04. Floating-point square-root estimates are corrected with integer comparisons before accepting a square. No external mathematical libraries are required.

Command syntax:

```text
./s4 S_lo S_hi [threads] [log2_chunk] [--out FILE] [--ckpt FILE] [--resume] [--verify]
```

The interval is **[S_lo,S_hi)**. A fresh checkpoint must not already exist; resume an existing interval with the same bounds and chunk size. The checkpoint supplies its original thread count. A resumed chunk can repeat output records, so deduplicate five-sets when analyzing resumed data. A complete checkpoint ends in `DONE`.

To reproduce the entire result, this shell loop uses adjacent intervals without the old endpoint gaps. **This is a long computation; it is not run by the verifier.** Use a new working directory for a fresh census:

```sh
mkdir -p reproduction
lo=1
frontier=12000000000000
step=5000000000
while [ "$lo" -lt "$frontier" ]; do
    hi=$((lo+step))
    [ "$hi" -le "$frontier" ] || hi=$frontier
    stem="reproduction/block-$lo"
    if [ -e "$stem.ckpt" ]; then
        nice -n 15 ./s4 "$lo" "$hi" 4 20 --verify --out "$stem.out" --ckpt "$stem.ckpt" --resume || exit 1
    else
        nice -n 15 ./s4 "$lo" "$hi" 4 20 --verify --out "$stem.out" --ckpt "$stem.ckpt" || exit 1
    fi
    lo=$hi
done
```

The example uses four low-priority workers for convenience; thread count is not part of the mathematical bound. Recorded timings used different counts and scheduling policies, so they are not a fixed runtime promise. The final exhaustive phase ran from 2026-10-14 19:49:44.471532 to 22:59:50.182049 UTC (3h10m05.711s), with ten low-priority workers under adaptive CPU affinity. It added 237.2 billion S at about 74.868 billion S per hour. This timing excludes subsequent heuristic work and final reporting. This package does not install the original Windows CPU monitor.

## 6. Files

| Path | Purpose |
|---|---|
| `s4.c` | Current, unchanged continuation source |
| `verify.py`, `compact.py` | Portable audit, with optional rechecking of omitted bulk outputs |
| `verification.json` | Successful package verification record |
| `SHA256SUMS`, `.gitattributes` | File integrity and preservation of exact bytes |
| `make_checksums.py`, `test_verify.py` | Regenerate checksums after intentional edits; test the portable reader |
| `analyze.py`, `analysis_1e12.txt` | Corrected historical result analysis |
| `logs/` | Original historical completion logs and all nine boundary repairs |
| `evidence/all_1e12.out.gz` | Full historical raw output; SHA refers to its decompressed bytes |
| `evidence/exhaustive-2026-09-{16,18,19,20}.tar.gz` | Four original continuations, including full raw outputs, preserved unchanged |
| `evidence/*-compact.tar.gz` | Eleven later continuations: completion records, all multi-extension records, recorded full-raw checks; bulk five-set outputs omitted |
| `pack_compact.py` | Rebuild compact archives from the original local run tree; no search |
| `evidence/verify_continuation.original.py` | Original independent verifier, retained byte-for-byte; mathematical helpers reused by `verify.py` |
| `evidence/s4-historical.c` | Historical source snapshot |
| `evidence/historical-data-verification.json` | September 15 recheck of the historical data |
| `evidence/analysis_1e12.legacy.txt` | Superseded historical analysis, preserved for provenance |
| `LICENSE` | MIT license |

Most of this package is the roughly 145 MB of compressed evidence already present in the earlier submission. This update adds about 3.1 MB of compact continuation archives (each under 900 KB) plus small text/code files. Include the `evidence/` directory when publishing. The omitted new bulk outputs are not needed by the default verifier; they are needed for its optional full-raw recheck. The executable files inside the archives are evidence for binary hashes; verification does not execute them. Regenerate `SHA256SUMS` if intentionally changing a covered file, and rerun verification before publishing that revision.

## License

MIT. Copyright (c) 2026 Maciej Siekierski. See [LICENSE](LICENSE).
