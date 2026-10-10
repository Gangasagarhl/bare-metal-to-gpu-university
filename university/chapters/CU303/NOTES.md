# CU303 — CUDA IV: streams, graphs, unified memory, profiling and machine code: author notes

Chapters F6-28 to F6-34, level L4. Labs are in `university/labs/F6-28` to `university/labs/F6-34`.

Every lab folder passes `university/labs/run_lab.sh university/labs/<ID>` with status 0. All seven were re-run at the end of this build (2026-10-10), after the last change to any listing; the table below is generated from the `.log` files of that final sweep. Each chapter fragment was checked with Python's `html.parser` (balanced tags, every id prefixed with the chapter id, no URLs, no `<script>`, no inline style, all 22 template sections in order, every `data-src`/`data-run` file present, no dangling internal anchors).

## Toolchain (as recorded in the logs)

- nvcc: Cuda compilation tools, release 12.0, V12.0.140 (Ubuntu package nvidia-cuda-dev 12.0.146~12.0.1-4build4); PTX/SASS listings target sm_80; cuobjdump, nvdisasm and nvprof from the same installation.
- g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0, `-std=c++20 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined` for CPU models (F6-31 `first_touch_noasan` is built without sanitizers on purpose, see below).
- Not installed: Nsight Systems (`nsys`), Nsight Compute (`ncu`). No NVIDIA GPU in the container.

## Listings run

**No GPU exists in the build container.** Every `.cu` program is really compiled (warning-free) and really run; the run stops at the first runtime call with the runtime's own `cudaErrorNoDevice`, and the chapter shows that output with an "untested on hardware" box giving the expected GPU output. No GPU time, bandwidth, fault count or device property appears anywhere in the course as a measurement. PTX/SASS listings, ptxas resource reports, fatbinary listings, `nvcc --help` excerpts and header excerpts are real tool output. CPU models (stream/timeline model, pipeline model, launch/capture models, managed-memory model, timeline statistics, roofline model, SASS counter) are the university's own code and ran for real. Expected-fail runs: none (no listing is meant to fail to compile).

| Chapter | Run (`.log`) | Exit | Status |
|---|---|---|---|
| F6-28 | counter_reset | 1 | expected no-device error (cudaErrorNoDevice); untested on hardware |
| F6-28 | default_stream_help | 0 | pass |
| F6-28 | default_stream_warning | 0 | pass |
| F6-28 | forensic | 0 | pass |
| F6-28 | stream_model | 0 | pass |
| F6-28 | streams_events | 1 | expected no-device error (cudaErrorNoDevice); untested on hardware |
| F6-29 | forensic | 0 | pass |
| F6-29 | overlap | 1 | expected no-device error (cudaErrorNoDevice); untested on hardware |
| F6-29 | pipeline_model | 0 | pass |
| F6-30 | capture_model | 0 | pass |
| F6-30 | graph | 1 | expected no-device error (cudaErrorNoDevice); untested on hardware |
| F6-30 | graph_api_help | 0 | pass |
| F6-30 | graph_forkjoin | 1 | expected no-device error (cudaErrorNoDevice); untested on hardware |
| F6-30 | graph_update | 1 | expected no-device error (cudaErrorNoDevice); untested on hardware |
| F6-30 | launch_model | 0 | pass |
| F6-31 | first_touch | 0 | pass |
| F6-31 | first_touch_noasan | 0 | pass |
| F6-31 | managed | 1 | expected no-device error (cudaErrorNoDevice); untested on hardware |
| F6-31 | um_model | 0 | pass |
| F6-32 | nvprof_help | 0 | pass |
| F6-32 | nvprof_run | 0 | pass; untested on hardware (no GPU) |
| F6-32 | nvtx_host | 0 | pass |
| F6-32 | nvtx_pipeline | 1 | expected no-device error (cudaErrorNoDevice); untested on hardware |
| F6-32 | stats_pageable | 0 | pass |
| F6-32 | stats_pinned | 0 | pass |
| F6-32 | timeline_pageable | 0 | pass |
| F6-32 | timeline_pinned | 0 | pass |
| F6-32 | timeline_stats | 0 | pass |
| F6-33 | forensic | 0 | pass |
| F6-33 | normalize | 1 | expected no-device error (cudaErrorNoDevice); untested on hardware |
| F6-33 | prec_div_help | 0 | pass |
| F6-33 | resources | 0 | pass |
| F6-33 | roofline | 1 | expected no-device error (cudaErrorNoDevice); untested on hardware |
| F6-33 | roofline_model | 0 | pass |
| F6-33 | roofline_points | 0 | pass |
| F6-33 | rounding | 0 | pass |
| F6-33 | sass_counts | 0 | pass |
| F6-33 | sass_fastdiv | 0 | pass |
| F6-33 | sass_normalize | 0 | pass |
| F6-33 | sass_poly4 | 0 | pass |
| F6-34 | arch_help | 0 | pass |
| F6-34 | fatbin_contents | 0 | pass |
| F6-34 | forensic_after | 0 | pass |
| F6-34 | forensic_before | 0 | pass |
| F6-34 | kernels | 1 | expected no-device error (cudaErrorNoDevice); untested on hardware |
| F6-34 | ptx_axpy | 0 | pass |
| F6-34 | rowsums | 1 | expected no-device error (cudaErrorNoDevice); untested on hardware |
| F6-34 | sass_axpy | 0 | pass |
| F6-34 | sass_blocksum | 0 | pass |
| F6-34 | sass_copy4 | 0 | pass |
| F6-34 | sass_lineinfo | 0 | pass |
| F6-34 | sass_nofma | 0 | pass |
Totals: 52 runs; 40 pass (one of them, F6-32 `nvprof_run`, ran nvprof which skipped GPU profiling for lack of a device), 12 CUDA programs compiled and run to the no-device error (untested on hardware), 0 unexpected failures.

## Per chapter

### F6-28 Streams and events
- Unverifiable claims: stream-to-hardware-queue mapping, number of hardware queues, false dependencies between streams (unverified box); vendor documents D1 etc. are titles only (gate G1). Header H1 (installed `cuda_runtime_api.h`) used for stream/event API behaviour; `nvcc --help` for `--default-stream`.
- Unverified boxes: hardware queue mapping; untested on hardware (Listing 1).
- Forensic evidence (counter reset) comes from the course's own stream model, not from a GPU trace.

### F6-29 Overlapping copies and compute
- Unverifiable claims: effect of issue order on GPUs with one copy engine, number of copy engines per GPU (no document opened; the model states it).
- Unverified boxes: issue order / copy engines; untested on hardware (`overlap.cu`).
- TG-1 copy/compute parameters used as teaching numbers, not hardware numbers.

### F6-30 CUDA graphs
- Unverifiable claims: that stream-captured kernel launches keep argument values as `cudaGraphAddKernelNode` does (header documents only the latter); GPU-side launch gap with graphs; host cost per launch. Executable graphs are kept alive until the end of Listing 4 because the header does not state that an executable graph is independent of the graph it was made from.
- Unverified boxes: captured argument copying; launch representation/cost; untested on hardware (Listings 1, 2, 4).

### F6-31 Unified (managed) memory
- Unverifiable claims: CPU access to managed memory during a kernel when `concurrentManagedAccess = 0`; migration granularity, fault cost, batching, generations supporting GPU page faults.
- Unverified boxes: concurrent access semantics; migration internals; untested on hardware (`managed.cu`).
- Forensic: the ASan build shows 8192 extra minor faults on the third pass (shadow memory, 1:8 of 256 MiB / 4 KiB pages); the clean build (`first_touch_noasan`) shows 0. Both are real CPU runs.

### F6-32 Profiling with Nsight Systems and NVTX
- Unverifiable claims: Nsight Systems command lines, option names, report formats, row names (tool not installed); nvprof support status for current GPUs; timestamp generation and tracing overhead.
- Unverified boxes: Nsight Systems workflow; nvprof status; timestamp/overhead; untested on hardware (`nvtx_pipeline.cu`).
- NVTX behaviour taken from the installed `nvtx3/nvToolsExt.h` (H2), including "close to zero overhead if no tool is attached".

### F6-33 Nsight Compute and the roofline
- Unverifiable claims: Nsight Compute section and metric names, replay modes, CLI options (tool not installed); FP32/SFU unit counts and MUFU.RCP cost per generation.
- Unverified boxes: Nsight Compute names; unit counts and TG-1 compute roof; untested on hardware (`roofline.cu`, `normalize.cu`).
- Forensic (slow division): real SASS of IEEE division (`MUFU.RCP`, FFMA sequence, `FCHK`, `CALL` slow path) versus `--prec-div=false`, and a real CPU run (`rounding.cpp`) showing 332 of 1000 values differ between x/3 and x*(1/3).

### F6-34 Reading PTX and SASS
- Unverifiable claims: meanings of SASS details inferred by lining up SASS with PTX and source (R1 as stack pointer, `c[0x0][0x0]` = blockDim.x, parameter offsets from 0x160, `ULDC.64 c[0x0][0x118]`, `HFMA2.MMA` as a constant loader for 4, `.CONSTANT`/`.E` suffixes) — all marked "inferred"; driver JIT of PTX (Programming Guide, title only); instruction latencies, register-file size, local-memory caching.
- Unverified boxes: SASS meanings (inferred); per-architecture costs; untested on hardware (`kernels.cu`).
- Forensic (kernel slower after a compiler update): injected fault is the build option `-maxrregcount=16` (ptxas raises it to the sm_80 lower bound of 24; 24-byte stack frame, 68 B spill stores, 48 B spill loads; 17 STL / 12 LDL.LU, 6 + 6 inside the loop). Stated in the answer key and in source R11.

## Decisions for the owner

1. **Model-generated profiler evidence.** F6-28, F6-29, F6-32 and F6-33 forensic labs (stream_model, pipeline_model, timeline_stats, roofline_model with `forensic.in` inputs) and the F6-32 timeline figures use evidence produced by the course's own CPU models, labelled as such, instead of real Nsight Systems / Nsight Compute exports. Replace with real exports from a GPU run when hardware is available.
2. **Real GPU runs.** All 12 CUDA programs are untested on hardware; each chapter states the expected output. A GPU pass (record GPU, driver, toolkit) should replace the untested boxes.
3. **TG-1 compute roof (F6-33).** The chapter proposes a compute roof for the teaching GPU: 4 SMs × 32 lanes × 1 FMA × 2 FLOP × 1 GHz = 256 GFLOP/s, with the existing 256 GB/s giving a ridge point of 1 FLOP/byte. This extends HW301's TG-1 definition and needs owner approval (or the HW301 author's).
4. **Analogy mappings (F6, the great kitchen hall).** Proposed new mappings: order spikes and tokens for streams/events (F6-28); loading dock and storeroom for copies and pinned memory (F6-29); a laminated card for a CUDA graph (F6-30); runners fetching crates on demand for managed memory (F6-31); the wall chart for a timeline profiler (F6-32); the inspector for a kernel profiler (F6-33); the steward's hall-specific card for SASS (F6-34). Add to the analogy registry or replace.
5. **nvprof.** F6-32 shows nvprof's help and a run that skipped GPU profiling; whether to teach nvprof at all for current GPUs depends on NVIDIA's support status, not checked here.
6. **Glossary merges.** `glossary.json` includes copies (with CU303 chapter lists) of HW301's "Copy engine / DMA", "TG-1 (teaching GPU)", "PTX / SASS", "Spill / local memory" and SP302's "Memory-bound and compute-bound" so that the chapters' glossary links list CU303 chapters. Because entries merge by term with the first course alphabetically winning, CU303's own definitions of "Pinned (page-locked) memory", "Arithmetic intensity", "Roofline model" and "Ridge point" take precedence over HW301/SP302's; keep or reconcile.
7. **Sources.** All vendor documents are titles only (dossier gate G1 open); the Source Researcher must confirm them, in particular the CUDA Binary Utilities instruction set reference for F6-34's inferred meanings.
