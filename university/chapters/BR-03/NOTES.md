# BR-03 From CPU threads to GPU warps and wavefronts — author and lab notes

Bridge chapter (guide 6.2), level L2–L3, placed between F6-03 and F6-04 for readers coming
from SP203; referenced again by F7-08 (and already linked from F6-03, F6-12, F6-13, F7-08,
F7-09, F1-57, F1-58, F2-53, F2-54). Files: `BR-03.html`, `glossary.json` (5 new terms),
this file; lab in `university/labs/BR-03/`.

## Listings run (all by `university/labs/run_lab.sh university/labs/BR-03`, exit status 0)

Final run: 2026-10-10T02:38Z–02:39Z, g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0,
Cuda compilation tools release 12.0 V12.0.140, Linux x86_64 container with 4 CPUs, no GPU.

| Listing / step | Result |
|---|---|
| 1 `hist_threads.cpp` (ASan+UBSan correctness run) | pass, exit 0 |
| 1 `hist_threads_timing` (run.sh, -O2, "time") | pass, exit 0; measured on a shared VM |
| 2 `hist_cuda.cu` | built for real; run = `cudaErrorNoDevice`, exit 1 — **untested on hardware** |
| 2 `hist_resources` (ptxas -v, sm_80) | compiled only, exit 0 |
| 2 `hist_profiler` (nvprof 12.0.146) | "GPU profiling skipped" — **untested on hardware** |
| 3 `hist_racy.cc` plain ×5 / TSan | pass (deliberate race): lost 380,436–695,856; TSan exit 66 (its own code after a report) |
| 4 `spin_warp.cu` | built; run = `cudaErrorNoDevice`, exit 1 — **untested on hardware** |
| 4 `spin_sass` (sm_60, sm_80) | compiled only, exit 0 |
| 5 `simt_spin_model.cpp` (model) | pass, exit 0 |
| 6 `diverge.cu` + `diverge_sass` | built; host part prints no device, exit 0; SASS compiled only — **untested on hardware** |
| 7 `warp_model.cpp` (model) | pass, exit 0 |

No `.expect-fail` listings. `hist_racy.cc` is a `.cc` on purpose so that run_lab.sh does not
build it with the standard flags (same convention as F2-35).

**Timing numbers quoted in the chapter text** (8.0 ms, 100.0 ms, 372.9 ms, 30.6 ms, 16.3 ms,
63.3 µs, 15/18 context switches, 380,436–695,856 lost) come from the recorded run above.
Re-running the lab changes them; whoever re-runs must update the prose in Layer 2, the
Worked example, the Mistakes table, Figure 1 (63.3 µs, 8192 KiB) and the Summary, or keep
the recorded `.out` files.

Check-run outside the lab record: answer 4 of Check yourself (Listing 7 with `>= 250`) was
verified by compiling a modified copy in the scratch directory: 72.7 % (width 32) and
100.0 % (width 64) mixed groups. The answer says so.

## Unverified boxes (AH-18)

1. Layer 3, execution model: independent thread scheduling from compute capability 7.0 and
   whether it guarantees `countSpinLock` finishes; the pre-Volta deadlock; the meaning of
   SASS `SSY`/`SYNC`, `BSSY`/`BSYNC`, `YIELD`; AMD forward-progress rules. Check: CUDA C++
   Programming Guide ("SIMT Architecture", "Independent Thread Scheduling", warp vote
   functions), CUDA Binary Utilities, HIP Programming Guide, CDNA ISA reference.
2. Layer 3, profilers: Nsight Compute / Nsight Systems section and metric names (branch
   efficiency, sectors per request, atomic throughput) and nvprof GPU support. Check: Nsight
   Compute Kernel Profiling Guide and metrics reference; Nsight Systems user guide.
3. Code walk-through, Listing 2: untested on hardware; expected per-kernel results stated.
4. Code walk-through, Listing 4: untested on hardware; expected results and the possible hang.

## Claims written from memory (tagged to title-only sources, G1 open)

- Warp = 32 (NVIDIA), wavefront = 64 on CDNA, 32/64 on RDNA (D1, D3, D9) — the widths the
  compilers use are confirmed by build evidence X1 (F1-57's wavesize/sass logs).
- Consecutive threads by `threadIdx.x` form consecutive warps (D1).
- Resident warps keep registers on chip; switching costs nothing (D1; F1-60).
- `__syncthreads()` is block-only; grid barrier only with cooperative launch, and the launch
  fails if the grid cannot be co-resident (D1; F6-12).
- PTX memory consistency model with CTA/GPU/system scopes; `cuda::atomic_ref` with a thread
  scope in libcu++ (D10, D1) — the libcu++ detail is not in an unverified box; the Fact-Checker
  should confirm it or remove it.
- `RED` is the atomic form used when the old value is unused (interpretation shared with F6-13).
- CPU context switch mechanics (D8; F3-03); cost of contended atomics via cache-line movement
  (D7; F2-38, F1-36).
- Explanation of the slow skewed serial loop (repeated updates of one counter) is offered as
  a hypothesis in the Worked example, not tested.

## Decisions for the owner

1. **Length.** About 8,000 words of prose before Sources (the guide's L3 range is 4,000–7,000;
   other chapters are about 5,000). The bridge card asks for Carries over, Changes and Traps
   "each in depth" with a run per trap, plus two worlds of code; I kept the depth. If the
   Editor wants it shorter, the natural cut is the Layer 3 profiler paragraph and the
   Listing 5/7 tables (the models are explained in the prose).
2. **Analogy.** The chapter uses two registered worlds that are both kitchens: F2's restaurant
   kitchen (cooks, key to the spice cabinet, tally counter) for the CPU side and F6's great
   kitchen hall (helpers, rows, tables, far pantry) for the GPU side; the hook moves Chef
   Amara (F2-34) from one to the other. No new mapping was invented. Proposal for the
   registry: "spin lock in a warp = the helper with the key waits for her row; the row waits
   for the key" (used in the hook and the SIMT deadlock glossary entry).
3. **Course field.** Front matter `course: BR` (as instructed); the build places it under the
   bridges group.
4. **Profiler in the lab.** The card says "measure and explain ... with the profiler". With no
   GPU, the only profiler evidence is nvprof's real "GPU profiling skipped" output; the
   CPU side uses `getrusage` because `perf` does not work in this container. The GPU
   profiling step of the Lab is written as a task with metric names left to the learner's
   version of the documentation (unverified box 2).
5. **E4 acceptance test** is quoted verbatim in the Lab's Verification; the provided
   programs test only the scrambled and skewed inputs, so the learner must extend them
   before claiming E4.
6. **sm_60 target.** The spin-lock SASS is shown for sm_60 (pre-Volta structure) and sm_80;
   CUDA 12.0 still compiles sm_60 without a warning in this build. A later toolkit may drop it.
7. **Forensic scenario symptoms** ("never returns on the oldest GPU", "slow on the newer
   one") are story, not evidence; the answer key says so. Evidence is real compiler output
   and model runs only.
