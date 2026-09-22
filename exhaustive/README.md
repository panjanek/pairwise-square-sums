# A115040: a computational lower bound for positive square-sum sextuples

**Result, completed 21 September 2026:**

> **a(6) > 1,190,800,000,000. In particular, a(6) > 10^12.**

Here a(6) is the least possible largest member of a set of six **distinct positive integers** whose 15 pairwise sums are perfect squares, if such a set exists. This is a computational exclusion, not a determination of a(6) or a proof that no positive sextuple exists at any size.

The [OEIS A115040 entry](https://oeis.org/A115040), checked on 22 September 2026, still states the earlier bound a(6) > 10^9. This repository supplies the proposed improvement, source code, raw results, completion logs, and reproducible evidence checks.

## 1. What was searched, and why it proves the bound

Write a candidate sextuple in increasing order as

    0 < a < b < c < d < e < f,    S = a+b+c+d.

The completed census excludes every sextuple with **S < 4,763,200,000,000**. The two largest members e and f are not independently capped by the search.

Since a, b, c are all smaller than d, we have S < 4d. Thus d <= 1,190,800,000,000 would imply S < 4,763,200,000,000, contradicting the exhaustive exclusion. Consequently d, and therefore f, must exceed **1,190,800,000,000**.

In particular, **there is no sextuple whose six members are all at most 10^12**. This includes the endpoint 10^12 and is stronger than saying only that the members are below 10^12. The distinction between S (the sum of four members) and f (the largest member) is essential: the older census to S < 10^12 alone gave only the bound 2.5*10^11.

### Coverage ledger

All intervals below are half-open: the left endpoint is included, the right excluded.

| Evidence | Added S interval | Completed production blocks | Unique five-sets added |
|---|---|---:|---:|
| Historical census, with nine singleton repairs | [1, 1,000,000,000,000) | 10 original slices + 9 repairs | 1,374,167 |
| 16-17 September continuation, including its preflight bridge | [1,000,000,000,000, 1,954,600,000,000) | bridge + 192 | 759,709 |
| 18-19 September continuation | [1,954,600,000,000, 2,723,100,000,000) | 155 | 512,820 |
| 19-20 September continuation | [2,723,100,000,000, 3,512,100,000,000) | 159 | 471,454 |
| 20-21 September continuation | [3,512,100,000,000, 4,763,200,000,000) | 252 | 673,688 |
| **Total** | **[1, 4,763,200,000,000)** | **758 continuation blocks + bridge + historical coverage** | **3,791,838** |

No sextuple was found. All continuation blocks completed and both recorded audits passed for each continuation. The cumulative auxiliary catalog contains 740 primitive six-member near misses with exactly 14 square pair sums, including 477 whose total is square. Primitive means that no common square factor greater than one can be removed; it does not mean that the gcd is one. These counts are not a probability model for finding a sextuple.

## 2. Verify the supplied evidence (no search)

Requires **Python 3.10 or later**, standard library only. From this folder:

```sh
python3 -B verify.py --report verification.local.json
```

On Windows, `python` may replace `python3`. Do not use `-O`, which disables assertions used by the archived checker. The verifier uses one process, reads the compressed evidence, temporarily expands about 330 MB of records, and removes that temporary copy afterwards. It does not execute the archived binaries or start an exhaustive run. Allow a few minutes and roughly 1 GB of free memory. All paths used for verification are relative to this folder; there is no dependency on the author's WSL home directory or another repository.

The verifier checks:

- SHA-256 hashes from `SHA256SUMS`, then original source, binary, output, checkpoint, and predecessor hashes inside the manifests.
- Contiguous coverage from S = 1, including all nine repaired historical boundaries and the bridge [10^12, 10^12+10^9).
- Completed per-thread ranges, counters, `DONE` checkpoints, failure markers, and continuation capacity maxima.
- Positivity, distinctness, the reported S, and all ten pair sums of every stored five-set using exact integer square roots.
- Every stored multi-extension quadruple, all of its extensions, and every extension pair; any square remaining pair would be a sextuple and fails the claimed exclusion.
- Primitive near-miss catalogs, square totals, Euler classifications, novelty against preceding catalogs, and historical capacity bounds.

[verification.json](verification.json) records a successful verification of this package. It is a generated report, excluded from its own checksum list. `SHA256SUMS` preserves exact bytes of the supporting files; `.gitattributes` prevents line-ending conversion during a Git checkout. A locally generated `verification.local.json` is also excluded from the published checksum list.

### Scope of the computational evidence

This is an audit of the stored census and its coverage records, supported by the exhaustive algorithm and its tests. It is **not** a new implementation independently enumerating the entire historical range, and it is not a formal proof-assistant certificate. Checking reported solutions alone cannot prove that an enumerator omitted none; the completeness argument, implementation, coverage logs, and validation checks together support the computational bound. An editor can reproduce the enumeration using the supplied C source.

## 3. Historical repairs and implementation provenance

The original September 5 slicing script skipped S = k*10^11 for k = 1,...,9. Those nine singleton intervals were subsequently enumerated on September 15. Their logs and empty result files are included in `logs/boundary_*.err` and `.out`: 15 quadruples, no five-sets, no sextuples. The original logs are preserved as written; their gaps are explicitly filled, not silently edited away.

The historical raw file contains 1,374,180 five-set lines, including 13 duplicates, hence 1,374,167 distinct five-sets. `evidence/analysis_1e12.legacy.txt` is preserved as a historical artifact only: its old analyzer treated an entire multi-extension line as one configuration and reported 427 objects. Enumerating every pair of extensions correctly gives **430 primitive six-member near misses**, including **271 with square total**. The corrected `analyze.py` and current `analysis_1e12.txt` use that convention and make no discovery-probability claims.

The historical source snapshot is `evidence/s4-historical.c`. It had fixed-capacity arrays with silent truncation paths. The packaged verifier closes this specific capacity concern for S < 10^12:

- For any difference D < 10^12, the maximum divisor count is **6720**, below the array capacity 16384.
- For S < 10^12, the number of positive unordered representations as two squares is at most **192**, below 512.
- The complete historical raw output has at most **3 extensions per quadruple**. Reaching the old 64-extension limit would have emitted a 64-entry `M:` record, so the checked raw output excludes that truncation case.

The first two bounds are recomputed by finite enumeration of nonincreasing prime-exponent patterns. Moving larger exponents to smaller primes reduces the integer without changing its divisor count; for representations, only primes congruent to 1 modulo 4 contribute, with at most ceil(product(exponent+1)/2) positive unordered representations. See `capacity_bounds()` in `verify.py`.

The current `s4.c` is the exact source used by all four continuations. It aborts on representation, divisor, extension, and bucket overflows instead of silently truncating; records capacity high-water marks; and checks result writes and thread creation. It passed the recorded small-census equivalence, deliberately forced capacity failures, interrupted checkpoint/resume equivalence, and frontier-interval checks. The first continuation archive contains `preflight/summary.json` and the bridge output. At the final frontier the largest observed array usages were 256/512 representations, 4608/16384 divisors, and 3/64 extensions.

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
frontier=4763200000000
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

The example uses four low-priority workers for convenience; thread count is not part of the mathematical bound. Recorded timings used different counts and scheduling policies, so they are not a fixed runtime promise. The last continuation took 14h26m55s of search with ten low-priority workers under adaptive CPU affinity; both final audits completed inside its fifteen-hour budget. This package does not install the original Windows CPU monitor.

## 6. Files

| Path | Purpose |
|---|---|
| `s4.c` | Current, unchanged continuation source |
| `verify.py` | Portable audit of the entire packaged evidence |
| `verification.json` | Successful package verification record |
| `SHA256SUMS`, `.gitattributes` | File integrity and preservation of exact bytes |
| `make_checksums.py`, `test_verify.py` | Regenerate checksums after intentional edits; test the portable reader |
| `analyze.py`, `analysis_1e12.txt` | Corrected historical result analysis |
| `logs/` | Original historical completion logs and all nine boundary repairs |
| `evidence/all_1e12.out.gz` | Full historical raw output; SHA refers to its decompressed bytes |
| `evidence/exhaustive-*.tar.gz` | Four continuations: original manifests, audits, raw outputs, stderr, checkpoints, source snapshots, and archived binary |
| `evidence/verify_continuation.original.py` | Original independent verifier, retained byte-for-byte; mathematical helpers reused by `verify.py` |
| `evidence/s4-historical.c` | Historical source snapshot |
| `evidence/historical-data-verification.json` | September 15 recheck of the historical data |
| `evidence/analysis_1e12.legacy.txt` | Superseded historical analysis, preserved for provenance |
| `LICENSE` | MIT license |

The package is about 145 MB, principally compressed raw results. Include the `evidence/` directory when publishing. No raw data needs to be requested privately. The executable files inside the archives are evidence for binary hashes; verification does not execute them. Regenerate `SHA256SUMS` if intentionally changing a covered file, and rerun verification before publishing that revision.

## License

MIT. Copyright (c) 2026 Maciej Siekierski. See [LICENSE](LICENSE).
