# HP302 — HIP II: AMD GPU hardware, wavefronts and tools: author notes

This course has seven chapters, F7-08 to F7-14. F7-08 to F7-12 are at level L3; F7-13 and F7-14 are at L4.

- Labs are in `university/labs/F7-08` to `university/labs/F7-14`.
- Each lab passes `university/labs/run_lab.sh university/labs/<ID>` with exit status 0.
- All seven labs were re-run at the end of this build, on 2026-10-10.
- Every fragment passes the scratch validator. It checks:
  - html.parser balance;
  - ids prefixed with the chapter id;
  - no URLs and no `<script>`;
  - the 21 sections plus Answers, in order;
  - that every `data-src` / `data-run` target exists;
  - an inline SVG with sv-* classes only;
  - the forbidden phrases.
- `python3 university/build/build.py` reports no PROBLEM line for F7-08..F7-14, and none for any `#gl-` link they use. The remaining PROBLEM lines belong to other courses (MPI and parallelism terms).
- An earlier run of this task was interrupted by a container restart. Its F7-08 chapter and lab files were kept and corrected:
  - check question 7 and its answer were rewritten;
  - one forbidden phrase was removed.
- F7-09 to F7-14 were written or rewritten in this run. Several labs gained steps:
  - F7-10: `fir_fix`, `taps_isa`, `forensic_fix`;
  - F7-11: XOR swizzle in `transpose_lds.hip` and `lds_banks.cpp`;
  - F7-12: `mask32_quiet`;
  - F7-13: `options`, `bundle`.
- Where a quiz answer or worked example uses a number that is not in a recorded `.out` file, the text marks it "Lab Engineer's check" or "Lab Engineer's arithmetic". These were verified in scratch space (`/tmp/claude-0/work/HP302/`).

## Toolchain and local evidence (no GPU in this build)

- **Compilers:**
  - g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0, with the runner's flags;
  - hipcc, HIP 5.7.31921, driving Ubuntu clang 17.0.6 (`--offload-arch=gfx90a` unless stated; gfx1030, gfx1100 and gfx942 for comparisons);
  - nvcc 12.0 (F7-08 PTX comparison only).
- **Binary tools:** llvm-objdump-17, llvm-readelf-17, roc-obj-ls.
- **rocminfo 5.7.1-3build1.** It reports "ROCk module is NOT loaded, possibly no GPU devices" and exits with code 1.
- **Not installed:** rocprofv3, rocprof, rocprof-compute, omniperf, rocprof-sys-run, omnitrace, rocm-smi, amd-smi. This is recorded by the F7-14 `tools_present` step.
- **Header read in this build: H1, the HIP 5.7 headers.** The facts taken from them:
  - `warpSize`;
  - the wave64 `#error` guard in hip_runtime.h;
  - the `__launch_bounds__` expansion;
  - `__shfl*` built on ds_bpermute;
  - `__ballot` / `__lanemask_*` types;
  - `__syncthreads`.
- **Sources not opened.** No ISA guide, LLVM AMDGPU user guide, ROCm documentation page or profiler manual was opened. All D sources are cited "Title only — not opened during this build (dossier gate G1 open)".
- **No hardware numbers.** No hardware number of a real product appears unless the compiler printed it. Model values are labelled invented:
  - F7-09 SIMD-efficiency models;
  - F7-11 bank model;
  - F7-14 metrics.in and timeline.

## Listings run

Status key:

- **pass**: the run exits 0 with the expected output.
- **compiled only**: the step reads compiler or tool output, and no GPU is needed.
- **expected-fail**: a deliberate compile failure, recorded by the runner as "failed as expected".
- **untested on hardware**: the build is real; the run stops at its first runtime call with the runtime's no-device error (exit code 1). Each such run is covered by an unverified box.

| Chapter | Pass | Compiled only | Expected-fail | Untested on hardware |
|---|---|---|---|---|
| F7-08 | wave_model | lane_isa, ptx_warp, targets | — | wave_info, wave_info_cuda |
| F7-09 | simd_eff | w32_w64, ballot_isa | hip_w64 (HIP 5.7 rejects wave64 on gfx1030) | ballot |
| F7-10 | occ, occ_check | resources, spill_isa, taps_isa, forensic, forensic_fix | — | pressure, fir, fir_fix |
| F7-11 | lds_banks | lds_isa, lds_occupancy | — | transpose_lds, lds_occ |
| F7-12 | lane_sim, ops_count | ops_isa, mask32_quiet (no diagnostic with -Wall -Wextra) | mask32 (-Wshorten-64-to-32 -Werror) | wave_ops |
| F7-13 | — | dot_isa, loop_fma, options, census, code_object, kd, bundle | — | dot |
| F7-14 | metrics (invented inputs), timeline (model), tools_present | resources | — | timed; rocminfo (tool exit 1, no driver) |

## Unverified boxes (what the Source Researcher / a GPU run must settle)

- **F7-08:**
  - HIP 7.0 API changes;
  - SIMDs per CU, lanes per cycle, cycles per wave64 instruction, wave slots;
  - untested-on-hardware box (wave_info).
- **F7-09:**
  - wave64 on RDNA in later HIP;
  - meanings of v_cmpx, v_dual_* and MSG_DEALLOC_VGPRS;
  - RDNA wave64 execution and cycles;
  - untested-on-hardware box (ballot).
- **F7-10:**
  - occupancy calculator constants for gfx90a (512 registers per lane per SIMD, granule 8, 8 slots, 4 SIMDs, 64 KiB LDS). These were inferred from the compiler; they matched all 21 cases.
  - register file size and AGPR sharing; SGPR budget;
  - untested-on-hardware box.
- **F7-11:**
  - LDS bank count, width and service group;
  - LDS size per CU and per workgroup;
  - untested-on-hardware box.
- **F7-12:**
  - _sync variants in newer HIP;
  - readlane, DPP and ds_swizzle semantics;
  - inactive-lane behaviour of compares and bpermute;
  - untested-on-hardware box.
- **F7-13:**
  - dispatch-packet offsets in the prologue;
  - meaning of "unsafe" in -munsafe-fp-atomics;
  - .sgpr_count 17 vs .amdhsa_next_free_sgpr 13;
  - initial register state and packet formats;
  - untested-on-hardware box.
- **F7-14:**
  - all profiler command lines (none run);
  - counters, passes, replay and serialisation, roof measurement;
  - untested-on-hardware box (timed).

## Decisions for the owner

1. **No profiler in the build container.** The course's forensic ("ROCm kernel with low occupancy") and E10's "confirm in the profiler" use the compiler's resource report (`-Rpass-analysis=kernel-resource-usage`) as the evidence, labelled as such. Profiler confirmation is a GPU lab step (F7-14 step 6). Please decide:
   - whether to install rocprofv3 / ROCm Compute Profiler in the image;
   - or whether to provide recorded profiler output from a lab GPU.
2. **HIP version.** HIP 5.7 (Ubuntu package) is old. Several facts are version-bound and boxed:
   - the wave64-on-RDNA `#error`;
   - `__shfl` without `_sync`;
   - `__launch_bounds__` second argument = waves per EU.

   A newer ROCm in the image would change F7-09 and F7-12 evidence.
3. **Profiler tool names.** F7-14 uses the current names (ROCm Compute Profiler / ROCm Systems Profiler, formerly Omniperf / Omnitrace) and the commands `rocprof-compute` and `rocprof-sys-run`. These come from memory and must be checked against D7.
4. **Inferred occupancy model (F7-10).** It is presented as "inferred from the compiler, matches all cases". The alternative is to wait for the ISA guide or LLVM guide to be opened.
5. **Analogy mapping proposal for the F7 world (second building)**, for the analogy register:
   - wavefront = row of helpers (64 or 32);
   - SIMD = supervisor's counter;
   - VGPR = one hand of every helper;
   - SGPR = the row's shared notepad;
   - AGPR = tray for the matrix machine;
   - LDS = the table; LDS bank = drawer;
   - padding = empty slot at the end of a row of drawers;
   - swizzle = each row's own drawer order;
   - kernel descriptor = recipe header card;
   - kernarg = order slip;
   - the three profilers = door clipboard (rocprofv3), table counter (Compute Profiler), street film (Systems Profiler).
6. **Glossary overlaps.** `glossary.json` repeats four terms that HP301 or HP401 also define, with identical names so that the build merges their chapter lists:
   - LDS (Local Data Share);
   - AGPR (accumulation register);
   - warpSize (HIP);
   - Offload bundle (fat binary).

   The build keeps the first definition it reads.
7. **Course project.** The AMD hardware passport and tuning notes are assembled in F7-14's mini-project. Its fields are seeded across F7-08 to F7-13.
