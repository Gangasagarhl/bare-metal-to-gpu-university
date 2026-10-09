# SP302 Performance engineering on the CPU — author notes

Author / Lab Engineer run, build of 2026-10-09 (no internet; batch brief `AUTHOR_BRIEF.md` + `AUTHOR_BRIEF_UPPER.md`).
Level L3, 3 credits, faculty F2 (analogy world: the restaurant building and its kitchen). Prerequisites SP203, HW203, MA202.
Card: lab "optimise a CPU matrix multiply step by step with a speed table" (F2-56, prepares CU302); forensic
"The 2× regression" (F2-55); exam P (optimise a kernel, explain each gain with profiler evidence; the F2-55
mini-project uses that report format); project "a CPU SGEMM report with a roofline chart" (F2-56 mini-project).
Maps to: the Roofline paper (curriculum item 75; source D1 of F2-56) and Gregg's "Systems Performance" (D1/D2 in every chapter).

## Files

- Chapters: `F2-51.html` … `F2-56.html`. Each has all 21 sections plus answers (the forensic answer key is in Answers), two inline SVG figures, claim tags, a Transition box and an unverified box. Each fragment was checked for tag balance (html.parser), chapter-prefixed ids, no duplicate ids, no URLs and no `<script>`. Approximate word counts: 5,275 / 5,403 / 5,398 / 4,771 / 4,895 / 4,753.
- Glossary: `glossary.json` has 35 four-part entries generated from the chapters' Jargon boxes, so the two never disagree. Terms already defined by other courses are linked, not redefined: HW202 (branch prediction, misprediction penalty, 2-bit counter, cmov/select, speculative execution, flush, vectorisation, dead code elimination, optimisation level, dependency chain); HW203 (cache line, spatial locality, working set, row-major order, prefetching, memory mountain, TLB, false sharing, Little's law); HW301 (lane, peak bandwidth); HW205 (DVFS); SP202 (profiler, sampling profiler). In F2-55 these recaps are labelled "(recap from SP202)" and in F2-54 "(recap)". Every `#gl-…` link resolves.
- Labs: `university/labs/F2-51` … `F2-56`, each with sources, `run.sh`, and `.out` / `.log` files.

## Lab conventions (decision taken in this course)

- `.cpp` files are correctness programs that `run_lab.sh` builds with sanitizers.
- `.cc` files are timing programs that `run.sh` builds at `-O2` / `-O3` without sanitizers, because sanitizers would distort timings. `run_lab.sh` ignores `.cc` files.
- `run.sh` writes a `.log` record for each output: listing, toolchain, command, date, machine, exit code and note. Every timing log carries the AH-23 note "measured on the build container (a shared cloud virtual machine) … not a specification".
- `bench.hpp` is the shared timing harness from F2-51 (Listing 1 there). Byte-identical copies are in each lab folder, so that every folder runs on its own. md5 was checked: one distinct hash. If the harness is ever changed, change all six copies.
- `F2-56/roofline.in` is a **generated** file. `run.sh` writes it with `grep` from `peak.out`, `stream.out` and `sgemm.out`. It is kept because `run_lab.sh` feeds `.in` files to `.cpp` programs; `roofline.cc` is a `.cc` file, so `run_lab.sh` does not use it.
- Chapter prose describes recorded timings qualitatively ("several times", "about ten times") or quotes the recorded `.out` exactly. The labs were **not** re-run after the chapters were written, so every number in the prose matches the committed outputs. Re-running changes the timing outputs (noisy VM); if that happens, re-check the numbers quoted in the worked examples and answer keys:
  - F2-54: 0.560 / 4.335 ns.
  - F2-55: 16.33 / 26.40 ms.
  - F2-56: all roofs and the speed table.

## Listings run (`university/labs/run_lab.sh university/labs/F2-5x` from the repo root: all six exit 0)

Toolchain: `g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0`, `valgrind-3.22.0`, GNU gprof / objdump 2.42, Linux x86_64 shared 4-CPU VM ("Intel(R) Xeon(R) Processor @ 2.10GHz"; AVX2, FMA, AVX-512 reported).

| Chapter | Listings and evidence | Result |
|---|---|---|
| F2-51 | `bench.hpp`, `clockinfo.cpp` (sanitized), `dce.cc` (-O0 / -O2, plus objdump), `noise.cc` (quiet / busy), `stats.cpp` + `stats.in` (sanitized) | pass, exit 0 |
| F2-52 | `cacheinfo.sh`, `traverse.cc` (plus Cachegrind at n=1024), `chase.cc`, `transpose.cc`, `sharing.cc` (forensic) | pass, exit 0 |
| F2-53 | `kernels.cc` (vectoriser reports, objdump), `simd.cc` (3 builds), `order.cpp` (sanitized; forensic) | pass, exit 0 |
| F2-54 | `branches.cc` (cmov build and `-fno-if-conversion` jump build, objdump, Cachegrind `--branch-sim=yes`; forensic), `predictor.cpp` + `predictor.in` (sanitized) | pass, exit 0 |
| F2-55 | `counters.cpp` (sanitized), `hotspot.cc` (gprof, Callgrind), `orders_v1.cc` / `orders_v2.cc` (timed + Callgrind `--toggle-collect`; course forensic), `perf_try` | pass, exit 0. `perf_try.log` records perf's real failure (exit 2) and `orders_diff.log` records exit 1 (= files differ); both are recorded results, not lab failures |
| F2-56 | `peak.cc`, `stream.cc`, `sgemm.cc` (7 shapes × 5 steps all PASS, speed table at 512 and 1024), `roofline.cc` | pass, exit 0 |

- Expected-fail listings: none.
- **Untested on hardware:**
  - `perf` (stat / record / report): the wrapper reports "perf not found for kernel 6.18.44-fc", exit 2.
  - All hardware performance counters: `perf_event_open` returned errno 2 (ENOENT) for cycles, instructions, branch-misses and cache-misses. Software counters (task-clock, page-faults) worked.
  - Cachegrind/Callgrind and gprof stand in as the profiler evidence. Cachegrind's cache and branch numbers are simulations, and the chapters say so.

## Unverified boxes (one or more per chapter)

- F2-51: clock frequency / DVFS during the runs could not be observed (no cycle counter); the steady_clock step is observed, not specified.
- F2-52: cache sizes are as reported by lscpu inside the VM; the cause of the steep latency rise between 2 and 16 MiB despite a "260 MiB L3" is unknown; the prefetcher's behaviour is inferred, not documented.
- F2-53: vector unit count and width, instruction latencies and throughputs, and AVX-512 frequency effects are unknown. Intrinsic names were checked against the installed GCC 13 headers (H1); the Intel manuals were not opened.
- F2-54: predictor organisation, pipeline depth and penalty in cycles are unknown. The ~7.6 ns per misprediction is a measurement on this VM. Cachegrind's branch predictor is a model.
- F2-55: perf is untested on hardware. Also unexplained:
  - gprof reports 4,000,001 calls to `splitWords`, which is called once;
  - Callgrind shows `???:0x… [???]` rows. My guess is PLT stubs; this is flagged as a guess in the chapter.
- F2-56:
  - the FMA unit count and the frequency are unknown, so the theoretical peak cannot be compared with the measured 100.5 GFLOP/s;
  - 4-thread scaling is poor (1.68× FMA, 1.01× DRAM) and unexplained: SMT siblings, other tenants or a bandwidth cap are all possible;
  - arithmetic intensities use compulsory traffic.

## Deviations from the F2-51 protocol (stated in the outputs)

- The F2-51 protocol is a median of 21 runs after 3 warm-ups. Some programs use fewer runs:
  - `chase.cc`: median of 7 (sizes up to 1 GiB are slow);
  - `sharing.cc` and `peak.cc`: median of 11;
  - Callgrind / Cachegrind runs: 1 run (counts are deterministic; their printed times are meaningless, as the chapters note).
- `sgemm.cc` skips the naive step at n = 1024, for time.
- `stream.cc` counts 12 bytes per triad element (no write-allocate). The output header says so, and F2-56 Layer 3 discusses it.

## Analogy mapping (proposals for the registry)

- Reused from earlier courses: cache line = tray (HW203); false sharing = two recipes printed on one sheet (HW203); pipeline = row of stations; branch prediction = starting the regular's usual dish before the order arrives (HW202).
- New in SP302 (please register or reject):
  - benchmark run = stopwatch evening;
  - warm-up = preheating the stove;
  - SIMD = eight-spout ladle;
  - sampling profiler = manager glancing at the kitchen at random moments;
  - instrumenting profiler = clerk who logs every action;
  - roofline = stove limit versus pantry-door delivery limit;
  - arithmetic intensity = cooking steps per crate;
  - register tiling = several pans fed from each crate.

## Decisions for the owner

1. **Roofline intensity convention.** F2-56's tool uses compulsory traffic, so every SGEMM step sits at the same intensity. The chapter explains why this is an upper bound and misleading as a diagnosis. Measured DRAM traffic needs hardware counters (unavailable). Accept this, or ask for a Cachegrind-based traffic estimate as an extra listing?
2. **"The 2× regression" is 2.16× in instructions but about 1.6× in time** on the build VM. The forensic scenario keeps the card's title, and the answer key teaches reporting both. Keep it, or tune the program so that time is also about 2×?
3. **AVX2 roof versus AVX-512 roof.** The VM reports AVX-512, but the SGEMM micro-kernel is AVX2 (portable to more student machines), so the AVX2 peak is drawn as the roof and the AVX-512 peak as a dashed line. An AVX-512 micro-kernel is offered as a lab extension.
4. **perf.** Its content is written as untested on hardware. A machine with working counters should run the F2-55 lab before release to confirm the perf commands, and to measure real IPC and DRAM bytes for the F2-56 roofline.
5. **Shared `bench.hpp` copies** (six identical files). Alternative: one shared include folder, but the lab runner builds per folder.
6. **Links to chapters that do not exist yet** are given as plain text, not links: CU201, BR-03 (F2-54 meta), and "a topic for F11" (F2-54). The CU302 and SP302 course anchors are linked because those course pages exist.

## Build

`python3 university/build/build.py` ran to completion. None of its PROBLEM lines mention F2-51…F2-56 or SP302; the remaining PROBLEM lines belong to other courses (missing glossary terms of other faculties, an F2-44 lab).
