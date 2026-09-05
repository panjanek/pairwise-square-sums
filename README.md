# pairwise-squares: exhaustive search for sets of positive integers whose pairwise sums are all squares

`s4.c` is a single-file, multithreaded C program that finds **every** set of 5 or 6 distinct positive integers
with all pairwise sums perfect squares whose four smallest elements sum to less than a chosen bound `S_hi`.
It runs in time roughly proportional to `S_hi` (about 15× per decade in practice) instead of the quadratic
cost of a search ordered by the largest element, and it was written to attack the open question of whether a
**6-set** of positive integers exists (Erdős–Moser question, R. K. Guy, *Unsolved Problems in Number Theory*,
problem D15; OEIS [A115040](https://oeis.org/A115040)).

**Headline result (2026-09-05):** no set of six distinct positive integers with all pairwise sums square has
its four smallest elements summing to less than 10¹². Consequently no such 6-set has its third-largest element
below 2.5·10¹¹, and in particular **the smallest possible largest element of a 6-set, A115040(6), exceeds
2.5·10¹¹** (previous published bound: 10⁹, L. Blomberg 2014). The same run lists all 1 374 167 five-sets with
four-smallest-sum below 10¹² (404 661 of them not k² multiples of smaller ones) and 427 primitive
"pseudo-solutions" (six numbers with 14 of the 15 pair sums square).

Contents: [Build](#1-build) · [Run](#2-run) · [Algorithm and mathematics](#3-algorithm-and-mathematics) ·
[Validation](#4-validation) · [Results](#5-results-as-of-2026-09-05) · [References](#6-references)

---

## 1. Build

Linux, GCC (any version with `unsigned __int128`, i.e. every x86-64/ARM64 GCC or Clang):

```bash
gcc -O3 -march=native -o s4 s4.c -lm -lpthread
```

Tested with GCC 13.3 on Ubuntu 24.04 under WSL2 (Windows 11) on an Intel Core Ultra 7 265H (16 cores).
No dependencies beyond libc, libm and pthreads. `-march=native` is optional (5–10 %).

## 2. Run

```
./s4 S_lo S_hi [threads] [log2_chunk] [--out FILE] [--ckpt FILE] [--resume]
                                       [--noext] [--verify] [--dump4 N] [--test-sixset]
```

| argument | meaning |
|---|---|
| `S_lo`, `S_hi` | search all 4-sets `{a,b,c,d}` with `S_lo ≤ a+b+c+d < S_hi` (see §3 for what that covers). `S_lo ≥ 1`. Any split of a range into sub-ranges gives the same total output. |
| `threads` | worker threads; omitted or `0` = number of online CPUs. A resumed run adopts the count stored in its checkpoint (the per-thread S-regions depend on it). |
| `log2_chunk` | log₂ of the chunk width `W` of the S-bucketing (default 20, i.e. 1 048 576 values of S per chunk). 20–22 is a good range; it trades per-chunk overhead against cache footprint (~7 bytes per S per thread). |
| `--out FILE` | append result lines to FILE instead of stdout. |
| `--ckpt FILE` | write a checkpoint to FILE every 5 s (atomically, results fsynced first); mark it `DONE` at the end. Refuses to start if FILE exists unless `--resume` is given. |
| `--resume` | continue from FILE's checkpoint (same `S_lo S_hi log2_chunk` required). Exits at once if the checkpoint says `DONE`. A chunk interrupted mid-way is redone, so a few duplicate result lines can follow a crash (`sort -u` removes them). |
| `--noext` | enumerate 4-sets only (no extension step). Useful for timing the enumeration. |
| `--verify` | additionally re-check every generated 4-set (six square sums, distinct elements); prints `BAD 4-set` lines if anything is wrong (nothing ever was). Costs almost nothing. |
| `--dump4 N` | print the first N 4-sets of every thread as `4: [a,b,c,d] S=…` (debugging / data). |
| `--test-sixset` | drill: the first pseudo-solution found is pushed through the 6-set alert path (results file, `SIXSET_FOUND.txt`, stderr) clearly labelled `TEST-6SET (NOT A SOLUTION …)`. Verifies the persistence path without waiting for a real hit. |

### Output (stdout or `--out`)

```
5: [a1,a2,a3,a4,a5] S=…            a 5-set, largest first; S = a2+a3+a4+a5 (sum of the four smallest)
M: [a,b,c,d] ext=[e,f,…] S=…       a 4-set contained in two or more different 5-sets (a "pseudo-solution":
                                    {a,b,c,d,e,f} has 14 of 15 pair sums square, only e+f fails)
!!!!!!!! FOUND 6-SET: [………] S=…    a 6-set (never seen so far). Also written immediately with fsync to
                                    SIXSET_FOUND.txt in the working directory and to stderr.
```
Every printed set is re-verified with exact 128-bit arithmetic just before printing; a failure would append
`*** VERIFY FAILED ***` to the line (never happened). Each 5-set is printed exactly once, from its four smallest
elements. Elements of the sets can exceed 2⁶⁴ (the two largest elements of a 5-/6-set are bounded only by
`S_hi²/4`), hence the 128-bit arithmetic and decimal printing.

### Statistics (stderr)

A header line, a progress line every 30 s (`[123s] chunks done/total  4-sets …  5-sets …  6-sets …`), per-thread
lines and a final line

```
TOTAL: S_done=… pairs=… S_ge3=… triples=… 4-sets=… 5-sets=… 6-sets=… multi-ext-4sets=… ext_cand=… ext_sqrt=… wall=…s
```
(`S_done` values of S processed; `pairs` representations p²+q² enumerated; `S_ge3` values of S with ≥ 3
representations that were examined; `triples` triples of representations tried; `4-sets` positive integer 4-sets
found; `5-sets` reported 5-sets; `multi-ext-4sets` = number of `M:` lines; `ext_cand` divisor-pair candidates
for a fifth element; `ext_sqrt` exact square tests performed), followed by a histogram of the 5-sets by
⌊log₂(largest element)⌋.

### Examples

```bash
# 1 second: everything with four-smallest-sum < 10^7, with self-checks; 123 five-sets, among them the 16
# smallest known ones, e.g. [763442,177458,148583,28658,7442]
./s4 1 10000000 --verify

# 6 seconds: S < 10^9 (8899 five-sets, 59 pseudo-solutions)
./s4 1 1000000000 > out_1e9.txt

# a long run in resumable slices of 10^11 (each ~30-45 min on 16 cores); rerun the same loop after a crash
for i in $(seq 0 9); do
  ./s4 $((i*100000000000+1)) $(((i+1)*100000000000)) 0 20 --out r_$i.out --ckpt r_$i.ckpt --resume
done
grep -h "FOUND 6" r_*.out SIXSET_FOUND.txt 2>/dev/null
```

Resource guide (16 threads, Core Ultra 7 265H): `S_hi` = 10⁹ → 6 s, 10¹⁰ → 91 s, 10¹¹ → 25 min, 10¹² → 6.4 h,
10¹³ → ≈ 4 days (extrapolated, ×15 per decade). Memory: about 10–20 MB per thread plus shared tables of
~10·2√`S_hi` bytes (20 MB at 10¹², 200 MB at 10¹⁴). Internal limits: `S_hi` < 2⁶² and at most 512
representations per S (sufficient up to about `S_hi` = 10¹⁴; raise the `512` arrays in `process_chunk` beyond).

---

## 3. Algorithm and mathematics

### 3.1 Problem, notation, elementary facts

We look for sets `{a1 > a2 > … > an}` of distinct positive integers such that `ai + aj` is a perfect square for
every pair `i ≠ j`. Write `r_ij = √(ai + aj)` for the (integer) roots.

* **Scaling.** If `{ai}` works, so does `{k² ai}`. We call a set *primitive* if no `k² > 1` divides all its
  elements. Every set is `k²` times a primitive one.
* **Residues mod 4.** Squares are 0 or 1 mod 4. Checking all residue pairs shows that any such set with ≥ 3
  elements has all elements ≡ 2 (mod 4) except at most one ≡ 3, or all ≡ 0 (mod 4) except at most one ≡ 1.
  Hence at most one element is odd, and the sum of any four elements is ≡ 0 or 1 (mod 4), never 2.
* **Counting heuristics.** Modelling "is a square" as independent events of probability `1/(2√x)`, the expected
  number of n-sets with elements ≤ N behaves like `N^n · N^(−n(n−1)/4)`: linear in N for n = 4, logarithmic
  for n = 5, and `N^(−3/2)` (a convergent tail) for n = 6. Reality is much richer for n = 5 (§5), which is one
  motivation for looking further.

### 3.2 A 4-set is the same thing as three representations of one number as a sum of two squares

Let `{a,b,c,d}` be a 4-set and `S = a+b+c+d`. Pairing the elements in the three possible ways,

```
S = (a+b) + (c+d) = (a+c) + (b+d) = (a+d) + (b+c),
```
so `S` is a sum of two squares in three different ways. **Conversely**, take any number `S` and any three
distinct representations `S = x1 + (S−x1) = x2 + (S−x2) = x3 + (S−x3)` with `x1, x2, x3` squares (each
representation is an unordered pair of squares; `xi` is one member of pair i). Then

```
a = ( x1 + x2 + x3 − S) / 2        a+b = x1      c+d = S − x1
b = ( x1 − x2 − x3 + S) / 2        a+c = x2      b+d = S − x2
c = (−x1 + x2 − x3 + S) / 2        a+d = x3      b+c = S − x3
d = (−x1 − x2 + x3 + S) / 2
```
satisfies all six pair-sum conditions identically. Choosing the other member of a pair (replace `xi` by
`S − xi`) permutes the four values or produces the "complementary" 4-set; of the 8 choices exactly **two
essentially different 4-sets** arise: the one from `(x1, x2, x3) = (Q1, Q2, Q3)` and the one from
`(Q1, Q2, P3)`, where `Pi < Qi` are the two squares of representation i (this is Lagrange's and Nicolas'
observation, cf. MacLeod 2009, §1). So:

* **Integrality:** `a` is an integer iff `x1 + x2 + x3 ≡ S (mod 2)`; then all four are integers. With
  `pi = √Pi`, this reads: class `(Q,Q,Q)` needs `p1 + p2 + p3` even, class `(Q,Q,P)` needs
  `p1 + p2 + p3 ≡ S (mod 2)`.
* **Distinctness is automatic:** `a = b` would force `x2 + x3 = S`, i.e. representations 2 and 3 coincide.
* **Positivity** must be checked (`min(a,b,c,d) > 0`); it is the only real restriction.
* **Degenerate case:** if `S = 2p²`, one representation is `p² + p²`, `P = Q`, and the two classes coincide;
  the program generates that 4-set only once.
* `S ≡ 2 (mod 4)` (both `p, q` odd) never yields a positive integer 4-set (§3.1) and is skipped.

The number of representations of `S = 2^i · ∏ p_j^{n_j} · ∏ q_k^{m_k}` (`p_j ≡ 3`, `q_k ≡ 1 mod 4`) as a sum
of two squares is 0 if some `n_j` is odd and otherwise `(∏(m_k + 1) + δ)/2`, so three representations need,
e.g., three distinct primes ≡ 1 (mod 4) or a square of one times another. About 3 % of all S qualify.

**Consequence:** *listing all S < S_hi together with their representations lists every 4-set with sum below
`S_hi`*, in time proportional to the number of lattice points `p² + q² < S_hi`, i.e. `≈ 0.39 · S_hi`.

### 3.3 Enumerating S with its representations (bucketed, chunked, per thread)

The S-range `[S_lo, S_hi)` is cut into `threads` contiguous regions; each thread walks its region in chunks
`[S0, S0 + W)` and does four passes per chunk (function `process_chunk`):

1. **Count.** For every `p` with `2p² < S1`: starting from the smallest `q ≥ p` with `p² + q² ≥ S0` (kept in
   a per-`p` cursor `qnext[p]` so that no square roots are needed after the first chunk), step `q` upward
   while `p² + q² < S1`, incrementing `cnt[S − S0]` (16-bit counters). `S` is updated incrementally
   (`S += 2q + 1`).
2. **Prefix sums** of `cnt` give bucket offsets `off[]`.
3. **Fill.** Repeat the walk and store `p` into the bucket of its `S` (`q = √(S − p²)` is recomputed later,
   exactly, because `S − p²` is a perfect square).
4. **Process.** For every `S` with at least three representations (and `S ≢ 2 mod 4`), for every triple of
   representations and each of the two classes, apply the parity test, compute `a, b, c, d`, test positivity,
   and hand the 4-set to `handle_4set` together with its six roots (they are just the `p`'s and `q`'s).

Cost per chunk is `O(W)` plus `O(√S_hi)` cursor overhead, memory `≈ 7 W` bytes; the whole enumeration is
about 2 s per 10⁹ of S on 16 cores. It is much cheaper than the extension step below.

### 3.4 Extending a 4-set by a fifth element with divisor pairs

A fifth element `e` must satisfy `e + a = u²` and `e + b = w²` for two of the elements, hence

```
(u − w)(u + w) = a − b =: D .
```
So `u = (f + g)/2`, `w = (g − f)/2` for a factorisation `D = f · g`, `f < g`, `f ≡ g (mod 2)`, and
`e = u² − a`. Enumerating the divisor pairs of `D` therefore yields **all** candidates for `e`; each candidate
is accepted iff `e > 0`, `e ∉ {a,b,c,d}` and `e + c`, `e + d` are squares (`e + a`, `e + b` are squares by
construction). The same argument with `n − 1` arbitrary elements shows that *any* set extends in only finitely
many ways.

Two facts make this cheap:

* **`D` comes pre-factored.** With any third element `c`: `a − b = (a + c) − (b + c) = r_ac² − r_bc² =
  (r_ac − r_bc)(r_ac + r_bc)`, and both factors are at most `2√S < 2√S_hi`. A smallest-prime-factor table
  (`spf[]`) and a quotient table (`quot[m] = m / spf[m]`) up to `2√S_hi + 64` give the full factorisation
  of `D` by table lookups only (no divisions). For the same reason `D ≡ 2 (mod 4)` cannot occur.
* **Choice of the difference.** Any of the six differences of the 4-set can serve as `D`; the program picks
  the one with the fewest divisors, estimated as `d(r − r') · d(r + r')` from a precomputed divisor-count table
  (`ndivtab[]`). This cuts the number of candidates from 65–90 to 20–30 per 4-set on average.

Divisors are generated from the exponent vector in mixed-radix order, so the complementary divisor of
`divs[t]` is `divs[nd − 1 − t]` and no division is needed in the candidate loop. Candidates are pre-filtered by
the residues of `u² + (c − a)` and `u² + (d − a)` modulo 64, 63 and 65 (table lookups; because the sets are
arithmetically very coherent these filters only reject ~75 % instead of the ~99.98 % they would reject for random
numbers), and survivors are tested with an exact 128-bit integer square root. `e` can be as large as `D²/4`,
so `e` and the sums `e + c`, `e + d` are kept in 128 bits.

Non-primitive 4-sets **cannot** be skipped in this step even though their 5-sets are `k²` copies of smaller
ones: a 4-set whose four elements are all divisible by `k²` can still extend to a *primitive* 5-set (the
odd-one-out families with residues `(0,0,0,0,1)` mod 4, mod 3, mod 5, …; e.g. the four even elements of the
`(0,0,0,0,1)` mod-4 family are all divisible by 8). An earlier version of the program made this mistake and
lost 10 % of the 5-sets; the current version extends every 4-set and classifies primitivity afterwards.

### 3.5 From 5-sets to 6-sets; what gets reported

`handle_4set` collects all extensions `E` of a 4-set `Q = {a,b,c,d}`.

* Every `e ∈ E` gives the 5-set `Q ∪ {e}`. It is printed only when `e > max Q`, i.e. when `Q` consists of the
  four smallest elements; this makes every 5-set appear exactly once (from its canonical 4-subset).
* If `|E| ≥ 2`, `Q` lies in two or more different 5-sets; the `M:` line records `Q` and `E`. For any
  `e ≠ f` in `E`, `Q ∪ {e, f}` has 14 square pair sums; it is a 6-set iff `e + f` is a square, which is tested
  (128-bit) for every pair. Thus the `M:` lines are exactly Lagrange's *pseudo-solutions* (he called a set with
  14 of 15 square sums a pseudo-solution and tabulated many of them in 1981).

### 3.6 Coverage guarantee (why this is an exhaustive search)

* A 5-set `a1 > a2 > a3 > a4 > a5` is found from its 4-subset `{a2,a3,a4,a5}`, whose sum
  `S = a2+a3+a4+a5` is the smallest of its five 4-subset sums. **Every 5-set with `S_lo ≤ a2+a3+a4+a5 < S_hi`
  is found**, whatever `a1` is (`a1` may be as large as `S²/4`).
* A 6-set `a1 > … > a6` is found from `{a3,a4,a5,a6}`: `a1` and `a2` are both extensions of it and
  `a1 + a2` is a square. **Every 6-set with `a3+a4+a5+a6 < S_hi` is found.**
* Since `a3+a4+a5+a6 ≤ (N−2)+(N−3)+(N−4)+(N−5) < 4N` when the largest element is `≤ N`, a run to
  `S_hi = 4N` proves that no 6-set has largest element `≤ N`; equivalently a run to `S_hi` proves
  `A115040(6) > S_hi / 4`, and also that no 6-set has *third-largest* element `≤ S_hi/4` with the two largest
  elements unbounded — a region no search ordered by the largest element can reach.
* Splitting a run: the union of the outputs of `[S_lo, S_mid)` and `[S_mid, S_hi)` equals the output of
  `[S_lo, S_hi)`. (A run with `S_lo > 1` alone misses the `k²` copies of sets whose canonical 4-set sum is
  below `S_lo`, which is harmless.)

### 3.7 Complexity and measured cost

Enumeration is `O(S_hi)` plus polylogarithmic factors (the number of `S` with many representations); the
number of 4-sets with sum below `X` grows like `X^1.13` empirically (2.44·10¹¹ below 10¹²), and the divisor
extension costs about 0.8 µs per 4-set, which dominates. Overall the running time grows ≈ 15× per decade of
`S_hi`. For comparison, a search ordered by the largest element `a1` with a nested loop over squares costs
`Θ(a1)` per value of `a1`, i.e. `Θ(N²)` up to `a1 = N`: covering `a1 < 2.5·10⁹` that way would take years where
`s4` needs 91 s (and simultaneously covers all lopsided sets with `a3 < 2.5·10⁹`).

### 3.8 Checkpointing (crash safety)

With `--ckpt`, the main thread writes every 5 s: header (`s4ckpt v1`, `S_lo S_hi threads W`), one line per
thread (`next chunk start`, region end, cumulative counters) and, at the end, `DONE`. Positions are captured
first, then the results file is `fsync`ed, then the checkpoint is written to a temporary file, `fsync`ed and
renamed into place, so a checkpoint never claims a chunk whose results are not durable. Workers buffer the
output of a chunk and write it (with `fflush`) before advancing their position. `--resume` restores positions
and counters (the `TOTAL` line is cumulative over resumed runs; the histogram covers the last run only).
Tested by `kill -9` in the middle of a run: the resumed output equals the uninterrupted output, apart from a few
seconds of duplicated lines from the redone chunks.

### 3.9 What did *not* help

* **Congruence sieves.** For every modulus 3 ≤ m ≤ 64 tested, 98–100 % of the residue patterns of 4-sets extend
  to admissible residue patterns of 6-sets, and every residue class of `S` occurring for 4-sets also occurs for
  4-subsets of 6-sets. There is no cheap modular obstruction beyond the mod-4 facts of §3.1.
* **Skipping non-primitive 4-sets** (see §3.4) — incorrect.
* Further micro-optimisation: enumeration is already near memory speed; the extension step could gain at most
  another ~2×.

### 3.10 Code map (`s4.c`, ~360 lines)

| function | role |
|---|---|
| `isqrt64`, `isqrt128`, `is_sq128` | exact integer square roots / tests, 64- and 128-bit |
| `fact_add`, `gen_divisors` | factorisation by `spf`/`quot` lookups; divisors in mixed-radix order |
| `extend4` | §3.4: choose `D`, enumerate divisor pairs, filter, test; returns all extensions |
| `handle_4set` | §3.5: count, extend, print `5:` / `M:` / `FOUND` lines, 6-set test |
| `process_chunk` | §3.3: the four passes over one chunk of S |
| `worker` | per-thread region loop, output buffering, progress position |
| `write_ckpt`, `read_ckpt`, `ckpt_thread_count` | §3.8 |
| `format_set`, `verify_set`, `durable_alert`, `buf_append`, `flush_buf` | output helpers |
| `main` | arguments, tables (`spf`, `quot`, `ndivtab`, square residues), thread regions, progress/checkpoint loop, statistics |

---

## 4. Validation

* The enumeration contains the A115040 sets `{1,3}`, `{6,19,30}`, `{2,359,482,3362}` and
  `{7442,28658,148583,177458,763442}` (`./s4 1 10000 --noext --dump4 100 | grep 3362`).
* `--verify` runs re-check every generated 4-set (all 325 094 for `S_hi = 10⁷`, all 89 275 963 for 10⁹): no failure.
* The 16 five-sets with largest element ≤ 2 958 443 found by an independent brute force ordered by the
  largest element are reproduced exactly (and are complete: the S-method with `S_hi = 10⁷` finds precisely
  these 11 with `a1 < 2.5·10⁶`, plus 5 more up to 2 958 443, consistent with the brute force).
* MacLeod's Table 4 (the five smallest positive 5-sets, arXiv:0909.1666) and Kamenetsky's published near
  misses (primepuzzles.net Problem 70) are reproduced; the near misses turn out to be one primitive
  pseudo-solution scaled by 1, 4, 9, 16 plus one other.
* Each optimisation stage of the program was checked to produce identical sets of `5:` and `M:` lines at
  `S_hi = 10⁹` and `10¹⁰` (the stage that skipped non-primitive 4-sets failed this test and was discarded).
* Independent post-processing of the final 5-set list (S < 10¹²): all 1 374 167 distinct 5-sets pass an exact
  all-pairs re-check; the 4709 four-element subsets shared by two or more 5-sets were recomputed from the list
  and the 4733 remaining pair sums tested — none is a square, confirming the absence of 6-sets by a second route.

---

## 5. Results (as of 2026-09-05)

All runs on one machine (16 threads). "primitive" = not `k²` times a smaller set.

| `S_hi` | wall time | 4-sets | 5-sets (primitive) | pseudo-solutions, raw (primitive) | 6-sets |
|---|---|---|---|---|---|
| 10⁷ | 1 s | 3.3·10⁵ | 123 | 2 (1) | 0 |
| 10⁸ | 1 s | 5.7·10⁶ | 1 229 | 11 (4) | 0 |
| 10⁹ | 6 s | 8.9·10⁷ | 8 899 (4 384) | 59 (18) | 0 |
| 10¹⁰ | 91 s | 1.31·10⁹ | 54 209 (22 597) | 268 (68) | 0 |
| 10¹¹ | 25 min | 1.82·10¹⁰ | 286 968 (100 722) | 1 074 (195) | 0 |
| 10¹² | 6.4 h | 2.44·10¹¹ | 1 374 167 (404 661) | 3 873 (427) | 0 |

**Bound.** No 6-set of positive integers has its four smallest elements summing to less than 10¹²; hence
`A115040(6) > 2.5·10¹¹`, and no 6-set has third-largest element ≤ 2.5·10¹¹ regardless of its two largest
elements.

The file `analysis_1e12.txt` in this repository is the post-processing summary of the 10¹² run: counts, the
complete list of the 427 primitive pseudo-solutions with their failing pair, and the expectation estimates.

**Smallest 5-sets** (largest element first; the complete list below 2.5·10¹¹ is in the run output):

```
[763442,177458,148583,28658,7442]    [956018,559343,188882,104882,32018]   [1295042,348482,208034,104447,9122]
[1775106,1231650,281250,90850,7119]  [1946018,910082,322018,56207,30818]   [2149218,228546,127863,82818,23458]
[2170568,1439432,568457,250568,19832] [2175698,1028402,416402,116498,32498] [2205218,1554503,913538,464738,50786]
[2307362,1295042,208034,104447,9122] [2366658,1070658,405567,161442,23458] [2528082,864882,220882,145143,18]
[2610434,976802,327362,59522,17207]  [2763026,1008338,398258,288983,188498] [2854728,1656648,545608,214776,141633]
[2899058,2418578,1352786,657938,290738]
```

**Counting 5-sets.** Primitive 5-sets with largest element `< 2^b`, for `b = 20, …, 37` (complete, since
`2^37 < 2.5·10¹¹`): 2, 5, 22, 46, 86, 180, 325, 621, 1122, 1958, 3220, 5356, 8825, 14401, 23005, 36396,
57280, 88957. The per-octave growth factor falls slowly from ≈ 2 to ≈ 1.55 (count ~ `N^0.65–0.9`), far above the
logarithmic growth of the independence heuristic. 5-sets are lopsided: median `a1/a2 = 3.1`, 17 % have
`a1 > 10·a2`; the largest `a1` seen is 1.5·10¹⁶.

**Pseudo-solutions** (six numbers, 14 of 15 pair sums square). Primitive ones with 4-set sum in
`[10⁶,10⁷), [10⁷,10⁸), …, [10¹¹,10¹²)`: 1, 3, 14, 50, 127, 232 (growth slowing from ×3.6 to ×1.8 per decade).
Smallest: `{9122, 104447, 208034, 348482, 1295042, 2307362}` (only 348482 + 2307362 fails),
`{334226, 515858, 641918, 3155198, 3835538, 7629458}`, `{1195794, 3023122, 5491602, 10644687, 26115282, 48845682}`.
269 of the 427 have a perfect-square total sum (chance level ≈ 10⁻⁵); all six known 6-sets of integers with a
negative element (Lagrange 1981, MacLeod 2009, Choudhry 2015) also have square total sums, while 0 of 404 661
primitive 5-sets do. Three primitive 4-sets have three extensions each (three 5-sets through one 4-set):
`{28860026400, 17124487200, 4431625200, 162503200}` with extensions 4981255200, 2553991200, 1674390681;
`{116877821832, 13920133768, 5692188168, 160061832}` with 20214645768, 9843938568, 2227921857;
`{333338307400, 27550240200, 22788964296, 815056200}` with 3112362880200, 56799344700, 3557194929.
None of the pairwise sums of these extensions is a square.

**Outlook.** Modelling a 6-set as a pseudo-solution whose failing sum happens to be a square, the expected
number of 6-sets among the pseudo-solutions found is 0.003 (naive density `1/(2√n)`) or 0.30 (density
multiplied by the number of square roots of the failing sum modulo 64·9·5·7·11·13, which accounts for the
arithmetic coherence of these sets). The residue-aware expectation grew by 0.14, 0.09, 0.05 and 0.02 over the
last four decades of `S_hi`, i.e. it is converging to about 0.32: the searched region already contains almost
all of the probability mass this kind of search can ever collect. Extending the search to 10¹³ (≈ 4 days) would
raise the bound to 2.5·10¹² but add only ≈ 0.02 to the expected number of 6-sets. If positive 6-sets exist in
quantity they will come from algebraic families (as the integer 6-sets with a negative element do: Lagrange's
parametrisation, two binary quartics/elliptic curves of rank 3–4), not from this region.

---

## 6. References

* OEIS [A115040](https://oeis.org/A115040) (minimum largest element of a positive n-set; 3, 30, 3362, 763442;
  "a(6) > 10^9", L. Blomberg 2014), [A292158](https://oeis.org/A292158) (least n-tuples),
  [A195854](https://oeis.org/A195854) (integers, negatives allowed).
* J. Lagrange, *Cinq nombres dont les sommes deux à deux sont des carrés*, Séminaire Delange-Pisot-Poitou 12
  (1970–71), exposé 20, 10 pp. — [free on NUMDAM](https://www.numdam.org/item/SDPP_1970-1971__12__A14_0/).
* J. Lagrange, *Six entiers dont les sommes deux à deux sont des carrés*, Acta Arithmetica 40 (1981) 91–96,
  [doi:10.4064/aa-40-1-91-96](https://doi.org/10.4064/aa-40-1-91-96) — free (CC-BY) at IMPAN, also via
  [EuDML 205799](https://eudml.org/doc/205799). First 6-set of integers (one negative), pseudo-solutions.
* J.-L. Nicolas, *6 nombres dont les sommes deux à deux sont des carrés*, Mémoires SMF 49–50 (1977) 141–143 —
  [free on NUMDAM](https://www.numdam.org/item/MSMF_1977__49-50__141_0/). The 5-set {7442, …, 763442}.
* A. R. Thatcher, *Five integers which sum in pairs to squares*, Math. Gazette 62 (1978) 25–29.
* A. J. MacLeod, *On sets of integers where each pair sums to a square*, [arXiv:0909.1666](https://arxiv.org/abs/0909.1666)
  (2009). The 4-set/S-representation method and the divisor extension as used here; three more integer 6-sets.
* A. Choudhry, *Sextuples of integers whose sums in pairs are squares*, Int. J. Number Theory 11 (2015) 543–555,
  [doi:10.1142/S1793042115500281](https://doi.org/10.1142/S1793042115500281). Infinitely many integer sextuples.
* R. K. Guy, *Unsolved Problems in Number Theory*, 3rd ed., Springer 2004, problem D15.
* C. Rivera, [Prime Puzzles Problem 70](https://www.primepuzzles.net/problems/prob_070.htm) (D. Kamenetsky's
  near misses, 2017).
* C. Elsholtz, L. Wurzinger, *Sumsets in the set of squares*, Quart. J. Math. 75 (2024) 1243–1254 (states that no
  positive 6-set is known).

## License

MIT, see `LICENSE`.
