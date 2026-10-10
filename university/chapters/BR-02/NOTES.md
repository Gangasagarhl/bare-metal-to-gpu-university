# BR-02 From CUDA to HIP: a second kitchen building with the same recipes — author and lab-engineer notes

Chapter `BR-02.html` (L3, placed before F7-01), lab in `university/labs/BR-02`, glossary in `glossary.json`
(2 new entries: "Warp-size-agnostic code", "Drop-in equivalent (library)"; every other term links to an
existing entry of CU201, CU301, HW301, HP301, HP302 or HP401).

Build container: no GPU, no internet. HIP 5.7.31921 (hipcc 5.7.1-3, clang 17.0.6), nvcc 12.0 (V12.0.140),
g++ 13.3.0, Python 3. `hipconfig --platform` = amd. hipify-perl/hipify-clang and rocprof/rocprofv3 are **not
installed**; compute-sanitizer, cuobjdump, nvdisasm, roc-obj-ls, rocminfo are. cuBLAS, CUB and Thrust headers are
installed; no rocBLAS, hipBLAS, hipCUB, rocPRIM or rocThrust.

`university/labs/run_lab.sh university/labs/BR-02` passes (exit 0), re-run after the last change.

## Listings and runs

| Run | File / step | Result |
|---|---|---|
| R1 | `saxpy_neg.cu` (starter, nvcc) | built; run exit 1 `cudaErrorNoDevice` — untested on hardware |
| R2 | `run.sh` step `tools` | pass (toolchain self-report, installed-tools check) |
| R3 | step `hipmap` (reuses `labs/F7-03/hipmap.py`) | pass |
| R4 | step `hipify` (reuses `labs/F7-04/toyhipify.py`, the university's teaching translator, not AMD's HIPIFY) | pass |
| R5 | step `port_amd` | **expected fail** (`__ballot_sync` undeclared for gfx90a); logged "failed as expected" |
| R6 | step `port_nvidia` | built; run exit 1 `cudaErrorNoDevice` — untested on hardware |
| R7 | step `naive_amd` | built silently; run exit 1 `hipErrorNoDevice` — untested on hardware |
| R8 | step `naive_warn` (`-Wshorten-64-to-32`) | pass, warning printed (device and host pass) |
| R9 | step `naive_nvidia` | built with nvcc/ptxas deprecation warnings for non-sync `__ballot` |
| R10 | `saxpy_neg_fixed.hip` (gfx90a) | built; run exit 1 `hipErrorNoDevice` — untested on hardware |
| R11 | step `fixed_nvidia` | built; run exit 1 `cudaErrorNoDevice` — untested on hardware |
| R12 | steps `isa_naive`, `isa_fixed`, `isa_summary` (reuse `labs/F7-07/isa_of.sh`) | pass (real AMD ISA, gfx90a and gfx1030) |
| R13 | step `ptx_summary` | pass (real PTX, sm_80) |
| R14 | `lanes_model.cpp` (CPU lock-step model) | pass, full run under ASan/UBSan |
| R15 | `tuning_model.cpp` | pass |
| R16 | step `libs` | pass |
| R17 | step `cublas_hipify` (`extra/uses_cublas.cu`) | **expected fail** of the AMD build of the translated file; nvcc -lcublas build of the original succeeded |
| R18 | `reverse_lds.hip` (gfx90a; run exit 1 `hipErrorInvalidDevice`, untested on hardware) and step `lds` | pass |

Generated files kept in `gen/` (translator output and the sed-made naive port) so that learners can read them.
The lab depends on three earlier lab tools by relative path: `../F7-03/hipmap.py`, `../F7-04/toyhipify.py`,
`../F7-07/isa_of.sh` (read only, not modified). If those folders change, re-run this lab.

Numbers in the text that came from computation: Worked example (256 vs 192 per block) matches R14's fourth data
set (1,048,576 vs 786,432 = 4,096 blocks × 256 / × 192). Check-yourself Q5 answer (lanes 16–47: naive and fixed
both 524,288 at n = 1,048,576 and 500,000 at n = 1,000,003) was verified by the author with a scratch copy of
Listing 3 plus that data set (not kept in the lab folder, so that the learner's run is the evidence).

## Unverified boxes

1. Whether HIP releases newer than 5.7 provide `__ballot_sync` / other `_sync` warp functions on AMD (only the
   installed 5.7.1 headers were read). Check D1.
2. No specific cuBLAS → AMD library function mapping and no profiler command line is given (nothing installed
   or openable). Check D7, D8, D2, D9.

Other untested-on-hardware statements: Lab step 8 and the predicted AMD output of the naive port (stated as a
prediction from R14); the code walk-through's hardware-requirement box.

## Claims tagged to documents not opened (gate G1 open)

D1 HIP docs (programming model, porting guide, API reference), D2 ROCm docs (compatibility matrix, profilers),
D3 CUDA Programming Guide (warps of 32 with consecutive thread ids, vote functions, shared memory), D4 MI200 ISA
guide (wavefront 64, EXEC, LDS, `ds_`, `s_bcnt1`), D5 RDNA ISA guides (wave32/wave64), D6 LLVM AMDGPU guide
(code-object metadata), D7 HIPIFY docs, D8 library docs, D9 Nsight/Compute Sanitizer docs, D10 PTX ISA
(`vote`, `popc`, `WARP_SZ`). All conceptual; every identifier in the listings is in an installed header (H1–H5)
or a real compiler run.

The reading of the AMD assembly in Layer 3 (compiler merges the leaders' identical atomics into one atomic of
value × number of active leaders: `s_bcnt1_i32_b64` of exec, `s_mul_i32`, one `global_atomic_add`) is the
author's interpretation of R12; it is labelled as such in a Note box and cross-checked by the CPU model (R14).

## Decisions for the owner

1. **Starter lab choice.** "Hipify a CU201 lab" was implemented with F6-04's SAXPY (milestone E1) extended by a
   warp-vote count of negative results, because no CU201 lab uses lane-level code where a warp-size assumption
   could be planted naturally. The vote itself is CU301 material (F6-15), listed as "recommended" in prereqs.
   Alternative: plant the assumption in a CU301 lab instead and make CU301 a hard prerequisite.
2. **Translator.** AMD's HIPIFY is not installed; the lab uses the university's own `toyhipify.py` (F7-04),
   labelled as such. When a real hipify-perl/hipify-clang is available, add its output beside R4 and R17.
3. **Analogy mappings used beyond the 8.1 registry** (proposal to register): "second kitchen building = AMD/ROCm
   platform", "shared cookbook language = HIP", "prep chef = hipcc" (as already used by F7-01 and the guide's
   chapter titles); "tally card with one box per seat = lane mask (ballot result)"; "row leader counting raised
   hands = elected leader lane doing the atomic add" (raising hands = warp vote is from F6-15).
4. **Length.** About 8,800 words including sources, tables' text and answers (prose alone is within the L3
   range, toward its upper end) because the bridge card asks for every carry-over, change and trap in depth with
   a run example each.
5. **Forensic production numbers** in the scenario are illustrative ("less than one in ten thousand"); the counts
   quoted in the key come from R14. The answer key is inline (per FRAGMENT_FORMAT), not in `_keys/`.
