# SE401 Performance engineering at scale — author notes

Author / Lab Engineer run, build of 2026-10-10 (no internet; batch brief `AUTHOR_BRIEF.md` + `AUTHOR_BRIEF_UPPER.md`; task file `university/build/prompts/SE401.txt`).
Level L4, 2 credits, faculty F12 (analogy world: building a house as a team; names used: Kwame, Ana, Yusuf, Ingrid). Prerequisites SP302, CU303 or DS402.
Card:
- Lab: "end-to-end profile of MP3 or MP2" (F12-14 lab; Listing 1 of F12-14 is the MP2 stand-in).
- Forensic: "The 99th percentile" (F12-14 forensic lab).
- Exam P: a performance review of a provided system. Every chapter's forensic lab practises it, and the F12-14 forensic lab is the model answer format.
- Project: a performance report for a mega project. The F12-15 mini-project assembles the five chapters' mini-projects into it.
- Maps to: Curriculum 13.4 (D2 in F12-11/13/14/15) and Gregg's "Systems Performance" (D1 in every chapter, title only).

## Files

- Chapters: `F12-11.html` … `F12-15.html`.
  - Each has all 21 sections plus answers, with the forensic answer key under Answers.
  - Each has two inline SVG figures (theme classes only, arrowheads as `sv-head` polygons, title + desc), claim tags, a Transition box and at least one unverified box.
  - Each fragment was checked with a local validator for tag balance (html.parser), chapter-prefixed ids, no duplicate ids, no URLs, no `<script>`, section order, source anchors and lab files.
  - Word counts (all text, tables included): 7,485 / 7,101 / 6,828 / 7,753 / 6,612. They are over the L4 target of about 5,000 words of prose because of the line-by-line tables and the forensic answer keys.
- Glossary: `glossary.json` has 30 four-part entries. They are generated from the chapters' Jargon boxes by a script, so the two cannot disagree.
  - Terms that other courses already define are linked, not redefined: percentile, tail latency, SLO, trace and span, metric (DS402); benchmark, microbenchmark, warm-up run, run-to-run variation, median and percentile, latency histogram, hot spot (SP302 / SP202); robust statistic, common cause (MA202); Little's law, Amdahl's law, utilisation, lock contention, mutex, context switch, strace, system call, blocking, starvation, fsync, journaling, N+1 redundancy, scalability, head-of-line blocking.
  - "USE method" is also defined by SE402 (F12-17). F12-14 introduces it first, so SE401 gives a four-part entry with the same term name. `build.py` merges entries with identical names, so there is one anchor.
- Labs: `university/labs/F12-11` … `F12-15`, with sources, `.out` / `.log`, and `run.sh` where real timing is involved (F12-12, F12-13, F12-14, F12-15).

## Lab conventions

These are the same as SP302:
- `.cpp` files are deterministic programs built by `run_lab.sh` with sanitizers, with `.timeout` files.
- `.cc` files are real timing programs built at `-O2` by `run.sh`, without sanitizers.
- `run.sh` writes `.log` records (listing, toolchain, command, date, machine, exit code, AH-23 note) and deletes its `.build` folder.
- Simulations use a SplitMix64 generator with fixed seeds and nearest-rank percentiles.
- Exercise data that is not measured says so in its first line: `F12-13/bootstrap.in`, `F12-15/usl.in`.

## Listings run

All five folders were run with `university/labs/run_lab.sh university/labs/F12-1x` from the repo root, and all exit 0. Every `.log` says `exit code: 0` (23 logs).

Toolchain:
- `g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0`
- `strace -- version 6.8`
- ThreadSanitizer from the same g++

The machine was a shared 4-CPU cloud VM ("Intel(R) Xeon(R) Processor @ 2.80GHz") with other agents running.

| Chapter | Listings and evidence | Result |
|---|---|---|
| F12-11 | `budget.cpp` (budget sums, fan-out), `reqcheck.cpp` + `reqcheck.in` (requirement checker), `signoff.cpp` (forensic) | pass (deterministic) |
| F12-12 | `queue_model.cpp` (M/M/1 vs simulation), `lockmodel.cpp` (forensic), `predict_real.cc` (real threads, `run.sh`), `predict_tsan` (TSan, no warnings) | pass |
| F12-13 | `omission.cpp`, `abstat.hpp` + `bootstrap.cpp` + `bootstrap.in`, `drift.cpp` (forensic), `ab_bench.cc` (real, `run.sh`) | pass |
| F12-14 | `kvsvc.cc` real service in modes whole / chunked / paced with `--key` analysis, `strace -f -c` run, TSan run (no warnings) | pass |
| F12-15 | `node_model.hpp` + `capacity.cpp`, `usl.cpp` + `usl.in`, `outage.cpp` (forensic), `scale.cc` piped into `usl` (real, `run.sh`) | pass |

- Expected-fail: none.
- Untested on hardware:
  - Linux `perf`: on-CPU and off-CPU sampling, flame graphs. It did not work in the container. No commands are given, and an unverified box says what to run where it works.
  - GPU measurement and profiling of MP3: F12-11/12/13/14/15 labs. There is no GPU.
  - Multi-machine MP2 profiling and capacity measurement: there is no second machine.

**Do not re-run the labs without re-checking the prose.** The deterministic outputs are stable. The real timing outputs change every run, and the chapters quote the recorded outputs exactly. After any re-run, re-check these:
- F12-12 Layer 3 and the worked example: `predict_real` offsets, and the U = 0.70 / 0.85 rows, which include a noise spike the text discusses.
- F12-13: `ab_bench` medians 673.10 / 143.91 ms, ratio 0.214 [0.209, 0.222], round ranges 520–826 / 140–166 ms.
- F12-14: every kvsvc number (all three modes, key files, strace counts) and Figure 2, which is drawn from the `whole` run's traces. The forensic lab and its key depend on the `whole` run's specific events: the slow WAL batch at 3004.2 ms and the snapshot hold of 23.07 ms at 3125.4 ms. A re-run will produce a different but analogous evidence pack, so the key would have to be rewritten.
- F12-15: the `scale_usl` numbers in Layer 3.

## Unverified boxes, per chapter

- **F12-11**:
  - Gregg (D1) and "The Tail at Scale" (D5) were not opened. The fan-out formula is verified by the build's own simulation, not by the source.
- **F12-12**:
  - The causes of the gaps between the M/D/1 prediction and the real `predict_real` run (constant offset, spike) are hypotheses.
  - D1 Gregg and D5 Harchol-Balter are title only.
- **F12-13**:
  - The attribution of "coordinated omission" (Gil Tene's talks) was not opened, so the term is defined from the chapter's own simulation.
  - Why `std::map` is the noisier variant: no hardware counters were available.
  - D1 Gregg, D4 Efron & Tibshirani and D5 Jain are title only.
- **F12-14**:
  - `std::mutex` fairness (ISO C++, title only), and whether the `chunked` run's worse tail involved re-acquisition by the snapshot thread: lock hand-offs were not recorded.
  - `strace -c` time-column semantics and `-w`, the meaning of the futex "errors", and the semantics of `fdatasync`, `getrusage(RUSAGE_THREAD)` and `CLOCK_THREAD_CPUTIME_ID` (manual pages, title only).
  - perf / off-CPU tooling is untested on hardware.
- **F12-15**:
  - Gunther's USL (D5) and Gregg (D1) are title only. The peak formula can be checked by hand.
  - Why `scale` throughput fell from 3 to 4 threads on the VM is unknown.

## Proposed analogy mappings

These are for the F12 registry. Only "design doc = the house blueprint" is registered.

- F12-11:
  - performance requirement = "20 °C in the living room at −10 °C outside, door opened 30 times an hour" versus "warm"
  - offered load = the children opening the door
  - latency budget = the building schedule split between teams
  - acceptance test = the inspector's thermometer
- F12-12:
  - performance model = estimating bricks per day before building
  - queue = bricklayers waiting at the one mortar mixer
  - service time = minutes per bucket
- F12-13:
  - benchmark = a fair timing contest between two bricklaying methods
  - interleaving = alternating methods morning and afternoon
  - coordinated omission = timing deliveries from when the truck left instead of when the order was placed
- F12-14:
  - whole-system profile = cards on every wheelbarrow from the lorry to the wall
  - off-CPU time = a bricklayer waiting at the closed gate
  - snapshot lock hold = the inspector closing the only gate to count
- F12-15:
  - capacity plan = hiring for the busiest week with one bricklayer off sick
  - safe rate = the pace at which walls stay straight
  - contention / coherency = sharing one mixer / telling each other what changed

## Decisions for the owner

1. **Noisy real runs.** The real timing labs ran on a shared VM with other agents. Some outputs have visible noise: `predict_real` U = 0.85 spike, `ab_bench` map rounds 520–826 ms, the `chunked` kvsvc run hit by slow fdatasyncs, and a poor USL fit for `scale`. The chapters use this noise as teaching material and say so. Decide whether to keep these recorded runs, or re-record them on a quiet machine and then update the prose listed above.
2. **Constructed exercise data.** `bootstrap.in` and `usl.in` are typed-in data, marked as constructed in the file and the prose, because the build machine cannot produce clean data for these methods. Confirm this is acceptable under AH-23.
3. **kvsvc as the MP2 stand-in.** The course lab ("end-to-end profile of MP2 or MP3") uses a single-process service with no network or replication. Decide whether the MP2 lab folder should later provide a real multi-node target.
4. **"USE method" defined twice** (SE401 F12-14 and SE402 F12-17) under the same term name. `build.py` merges them, and SE401's text is shown. Choose one wording.
5. **Length.** The chapters exceed the L4 prose target, mainly because of tables and answer keys. Trimming candidates: F12-14 Layer 3 (three-mode table plus discussion) and F12-11 worked example.
6. **Build.** `python3 university/build/build.py` reported no PROBLEM line for F12-11 … F12-15 or SE401. The only PROBLEM line was `duplicate id: gl-finite-state-machine-fsm`, from another course. The build rewrote `university/UNIVERSITY.html`.
