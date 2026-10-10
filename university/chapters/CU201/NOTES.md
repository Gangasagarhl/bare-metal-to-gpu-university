# CU201 — CUDA I: the GPU programming model: author notes

Chapters F6-01 to F6-09, level L2 (F6-01 marked L1–L2 as in the exemplar), 4 credits. Prerequisites: SP201, SP202, HW203; HW301 in parallel. Maps to Track E milestones E1 and E2, curriculum 13.2 topics 1–2 and the 13.4 measurement protocol.
Labs are in `university/labs/F6-01` to `university/labs/F6-09`.

Every lab folder passes `university/labs/run_lab.sh university/labs/<ID>` with status 0. All nine were re-run at the end of this build (2026-10-10), after the last change; the table below is generated from the `.log` files of that final sweep. `python3 university/build/build.py` reports no PROBLEM line for these chapters.

## Toolchain (as recorded in the logs)

- nvcc: Cuda compilation tools, release 12.0, V12.0.140 (Ubuntu package nvidia-cuda-dev 12.0.146~12.0.1-4build4); target sm_80 for PTX/SASS listings; cuobjdump from the same toolkit.
- Compute Sanitizer 2022.4.1 (it cannot start here: no injection library and no GPU, exit code 13, recorded honestly).
- HIP 5.7.31921-0, offload target gfx90a (F6-06 HIP slot of the harness only).
- g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0 with `-std=c++20 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined` for all CPU models.

## Listings run

**No GPU exists in the build container.** Every `.cu`/`.hip` program is really compiled (warning-free) and really run; the run stops at the first runtime call with the runtime's own `cudaErrorNoDevice` (or `hipErrorInvalidDevice`), and the chapter shows that output plus an "untested on hardware" box with the expected GPU output. No GPU time, bandwidth or device property appears anywhere in the course. PTX/SASS listings, ptxas resource reports, fatbinary listings and header excerpts are real compiler/tool output. The CPU models (thread-grid simulator, TG-1 launch-error model, coalescing model, bandwidth model, CPU benchmark harness) are the university's own code and ran for real.

| Chapter | Run (`.log`) | Exit / result | Status |
|---|---|---|---|
| F6-01 | blocks | 0 | pass |
| F6-01 | cuobjdump_help | 0 | pass |
| F6-01 | nvcc_help | 0 | pass |
| F6-01 | ptx | 0 | pass |
| F6-01 | vec_add | 1 | expected no-device error; untested on hardware |
| F6-01 | vec_add_unchecked | 0 | pass; untested on hardware (no GPU) |
| F6-02 | brighten | 1 | expected no-device error; untested on hardware |
| F6-02 | forensic | 0 | pass |
| F6-02 | grid_sim | 0 | pass |
| F6-02 | ptx | 0 | pass |
| F6-02 | stride_loop | 0 | pass |
| F6-02 | worked | 0 | pass |
| F6-03 | bins | 1 | expected no-device error; untested on hardware |
| F6-03 | bins_report | 0 | pass |
| F6-03 | bins_sass | 0 | pass |
| F6-03 | ptx_spaces | 0 | pass |
| F6-03 | resources | 0 | pass |
| F6-03 | sass_local | 0 | pass |
| F6-03 | spaces | 1 | expected no-device error; untested on hardware |
| F6-04 | fma_cpu | 0 | pass |
| F6-04 | sass_default | 0 | pass |
| F6-04 | sass_nofmad | 0 | pass |
| F6-04 | saxpy | 1 | expected no-device error; untested on hardware |
| F6-04 | vec_add_steps | 0 | pass |
| F6-05 | header | 0 | pass |
| F6-05 | host_call | compile error | expected-fail (must not compile) |
| F6-05 | launch_model | 0 | pass |
| F6-05 | oob | 1 | expected no-device error; untested on hardware |
| F6-05 | sanitizer | 13 | expected no-device error; untested on hardware |
| F6-05 | sanitizer_help | 0 | pass |
| F6-05 | scale | 1 | expected no-device error; untested on hardware |
| F6-05 | silent | 0 | pass; untested on hardware (no GPU) |
| F6-06 | async_model | 0 | pass |
| F6-06 | bench_cpu | 0 | pass |
| F6-06 | header_events | 0 | pass |
| F6-06 | timing | 1 | expected no-device error; untested on hardware |
| F6-06 | timing_hip | 1 | expected no-device error; untested on hardware |
| F6-07 | device_report | 1 | expected no-device error; untested on hardware |
| F6-07 | fatbin_fixed | 0 | pass |
| F6-07 | fatbin_sm90 | 0 | pass |
| F6-07 | gpu_code | 0 | pass |
| F6-07 | header_fields | 0 | pass |
| F6-07 | header_noimage | 0 | pass |
| F6-07 | header_versions | 0 | pass |
| F6-07 | report_check | 0 | pass |
| F6-08 | coalesce_model | 0 | pass |
| F6-08 | copy_kernels | 1 | expected no-device error; untested on hardware |
| F6-08 | forensic | 0 | pass |
| F6-08 | sass_copies | 0 | pass |
| F6-09 | bandwidth | 1 | expected no-device error; untested on hardware |
| F6-09 | bw_model | 0 | pass |
| F6-09 | forensic | 0 | pass |
| F6-09 | header_pinned | 0 | pass |
| F6-09 | host_copy | 0 | pass |
| F6-09 | rosa_copy | 1 | expected no-device error; untested on hardware |
| F6-09 | sass_vector | 0 | pass |

Values that vary between runs (F6-06 `bench_cpu` parts B–C, `async_model`; F6-09 `host_copy`) are not quoted exactly in prose; the text speaks of orders of magnitude only.

## Unverified boxes (claims written from memory; each names the document to check)

- **F6-01:** untested on hardware (vec_add expected output). HIP mapping kept as in the exemplar.
- **F6-02:** warp formation rule (x fastest), no promised block order, block-shape advice (Programming Guide: thread hierarchy, SIMT; Best Practices: execution configuration); brighten untested on hardware.
- **F6-03:** caches in front of local/global memory, shared L1/shared storage, constant-memory serialisation on divergent reads, meaning of ptxas `cmem[0]`/`cmem[3]`; spaces untested on hardware.
- **F6-04:** GPU FFMA = IEEE 754 fusedMultiplyAdd with round-to-nearest-even, therefore bit-identical to `std::fma` (Programming Guide FP appendix, NVIDIA "Floating Point and IEEE 754" note, PTX ISA `fma.rn`); SAXPY bit-exact result untested on hardware and depends on that statement.
- **F6-05:** 1,024 threads/block limit on current NVIDIA GPUs; Compute Sanitizer report format; scale.cu and the sanitizer run untested on hardware.
- **F6-06:** first-launch costs (context creation, lazy module loading); event timing untested; HIP event semantics assumed equal to CUDA's.
- **F6-07:** compatibility rules (SASS within major capability, PTX JIT for newer), driver JIT at start-up, AMD wavefront sizes; device report untested on hardware.
- **F6-08:** 32-byte sector / 128-byte line, cudaMalloc alignment, profiler "sectors per request" metrics and their values, caching softening small strides; copy-kernel table untested on hardware.
- **F6-09:** causes of the gap to the theoretical roof, vector loads helping copies, event-ordering argument for pageable copies; bandwidth table and pinned/pageable ratio untested on hardware.

All D-sources (CUDA C++ Programming Guide, Best Practices Guide, Runtime API, PTX ISA, NVCC, Binary Utilities, Compute Sanitizer, Nsight Compute, CUDA Samples, HIP/ROCm docs, Hwu/Kirk/El Hajj, Hoefler & Belli, Williams/Waterman/Patterson, IEEE 754-2019) are **title only — not opened during this build; dossier gate G1 open**. H-sources (installed CUDA 12.0 and HIP 5.7 headers) were read in this build and are quoted verbatim where quoted.

## Invented material (labelled as such in the chapters)

- **TG-1**, the university's invented teaching GPU (from HW301/F1-55): 4 SMs, 32-wide warps, 512 threads/block, 32 KiB shared/block, 256 GB/s peak, 32-byte segments. Used by the models in F6-02, F6-05, F6-07, F6-08, F6-09.
- Forensic evidence that cannot come from a real GPU here: Ama's timing table (F6-06), the profiler summary in "Slow by stride" (F6-08), Rosa's 1.25 ms (F6-09), report_check.in device values (F6-07). Each forensic lab says "How the evidence was produced".

## Decisions for the owner

1. **Lab GPU.** Choose the lab GPU (or cloud instance) and record toolkit and driver versions in the dossier; every "untested on hardware" box should be closed by one real run of each lab, replacing nothing in the prose but adding the real `.out`.
2. **Toolkit version gap.** The container has CUDA 12.0; the guide's source registry names the Programming Guide 13.x. The SASS/PTX shown is from 12.0 for sm_80; re-generate with the lab's toolkit and check that the line-number references in the code tables still hold.
3. **Compute Sanitizer.** It cannot run in the container; F6-05 lab step 3 must be validated on hardware, including the report format quoted from memory.
4. **HIP slot of the harness** (F6-06 `bench.h`, `timing_hip.hip`) is compiled only for gfx90a; confirm the target with the F7 courses.
5. **Bit-exact SAXPY (E1).** The acceptance test assumes FMA contraction on both sides (CPU reference uses `std::fma`). If the owner prefers a tolerance-based test, F6-04 Layer 3 and its forensic lab need adjusting.
6. **Analogy mappings (proposals for the registry, guide 8).** The chapters use the registered F6 mappings (helper = thread, row = warp, table = block, all tables = grid, hands = registers, table in the middle = shared memory, big pantry far away = global memory, fetching neighbouring jars in one trip = coalescing). These mappings are new and not yet in the registry; please accept, change or reject them:
   - constant memory = the menu board on the wall (F6-03);
   - local memory = a helper's private shelf in the far pantry (F6-03);
   - CUDA event = a slip in the order queue saying "stamp the clock when you get here"; warm-up = the first order of the morning while the ovens heat (F6-06);
   - compute capability = the generation of the hall's equipment; fatbinary = a folder of recipe cards printed for different kinds of hall (F6-07);
   - memory roof = jars per minute the pantry corridor carries when full; vector load = a crate of four jars (F6-09);
   - pinned memory = goods kept on the loading dock; pageable memory = goods in the ordinary warehouse, carried to the dock first (F6-09).
7. **Exemplar alignment.** F6-01 follows the guide-14 exemplar's shape; its PTX and runs are now real (the exemplar's placeholders are filled from this build's logs).
