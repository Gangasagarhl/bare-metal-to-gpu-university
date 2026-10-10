# CU302 CUDA III: from GEMM tiling to tensor cores — author's notes

This course has chapters F6-20 to F6-27 (levels L3–L4).

- **Labs:** `university/labs/F6-20` … `F6-27`.
- **Glossary:** `glossary.json` has 43 four-part entries, built from the chapters' jargon boxes. Their sources are "pending verification", except for terms backed by this build's own runs or headers. Terms that other courses' glossaries already define are not repeated.

## Build environment

- **Tools:** g++ 13.3.0 and CUDA 12.0 (`Cuda compilation tools, release 12.0, V12.0.140`), with nvcc, ptxas, cuobjdump and the cuBLAS headers and library.
- **No GPU.** Every `.cu` program that launches a kernel was really built. Its run records `cudaErrorNoDevice` (exit 1, `hardware: untested on hardware`).
- **Correctness evidence** comes from the university's CPU emulator, `labs/F6-21/cuda_shim.hpp` and `emu_test.hpp`. It runs one std::thread per GPU thread, uses a std::barrier per block, and makes cp.async copies land as late as is legal. It runs the real kernel source on 8 shapes, against the bound γ_K·Σ|ab|.
- **Performance evidence** is compiler output only (ptxas -v, PTX, SASS counts and loops), plus models: the NVIDIA occupancy calculator header `cuda_occupancy.h`, `latency_sim` and `pipeline_sim`.

All eight labs were re-run at the end with `university/labs/run_lab.sh university/labs/F6-2x`, and every run returned 0. `python3 university/build/build.py` reports no PROBLEM line for F6-20 to F6-27 or their `gl-` links. The PROBLEM lines that remain belong to other courses that are still in progress.

## Listings run

| Lab | Pass (exit 0) | Expected fail | Untested on hardware (built, run stops with no device) |
|---|---|---|---|
| F6-20 | occupancy, latency_sim, forensic, resources, sass_ilp | — | ilp, build_sm80 |
| F6-21 | check_naive (emulator), intensity, resources, ptx_naive, sass_naive | — | sgemm_naive, build_sm80 |
| F6-22 | check_tiled (emulator), resources, sass_tiled | forensic_tidy: exit 124, the 20 s timeout. This is intentional: a divergent `__syncthreads` deadlocks in the emulator. | sgemm_tiled, build_sm80 |
| F6-23 | check_regtile, occupancy_steps, resources, sass_mix, spill_report, spill_sass, spill (host only) | — | sgemm_regtile, build_sm80 |
| F6-24 | check_dbuf, forensic_fast, pipeline_sim, ptx_async, resources, sass_async, sass_mix | — | sgemm_dbuf, build_sm80 |
| F6-25 | formats (host-only .cu), tolerance, forensic_tol | — | — |
| F6-26 | coverage, fragments (host-only .cu), resources, ptx_wmma, sass_hmma, sass_loop, forensic_arch | — | wmma_gemm, build_sm80 |
| F6-27 | layout, hierarchy, forensic_layout | — | — |

## Unverified boxes, by chapter

- **F6-20:**
  - The example device's per-SM limits (2,048 threads, 32 blocks, 65,536 registers, 102,400 B of shared memory, 1,024 B reserved). These are a model, not a product.
  - The ILP benchmark is untested on hardware.
- **F6-21:**
  - Peak FLOP/s and bandwidth are product facts, and none are given.
  - The semantics of the cuBLAS calls (cublasCreate, cublasSetMathMode, cublasSgemm) and the row-major transpose trick.
  - The harness is untested on hardware.
- **F6-22:**
  - The per-SM limits and the maximum shared memory per block.
  - The SGEMM is untested on hardware.
- **F6-23:**
  - The alignment of `cudaMalloc`, `float4` alignment rules, and the local-memory caching path.
  - The SGEMM is untested on hardware.
- **F6-24:**
  - The cp.async .cg/.ca caching behaviour, and whether copies bypass registers. Only the header mapping was verified.
  - The SGEMM is untested on hardware.
- **F6-25:** how tensor cores accumulate internally (order, precision, rounding, subnormals) for each GPU.
- **F6-26:**
  - The alignment and ldm rules of load/store_matrix_sync.
  - Which compute capabilities support which types, and their throughput.
  - The semantics of `cublasGemmEx` with `CUBLAS_COMPUTE_32F`, and whether it uses tensor cores.
  - The error code a trap produces.
  - WMMA is untested on hardware.
  - The reading of "HMMA.16816" as m16n8k16 is an inference. It is stated in the text as an inference.
- **F6-27:**
  - All CUTLASS and CuTe API names and notation. CUTLASS is not installed and its docs were not opened.
  - The existence of a 16×8×16 instruction is inferred from the mnemonic.
  - The lab steps that use CUTLASS or a GPU.

## Sources

No document was opened. Every D source is marked "Title only — not opened during this build (dossier gate G1 open)". These include:

- NVIDIA: the CUDA C++ Programming Guide, the Best Practices Guide, the Nsight Compute docs, the PTX ISA, CUDA Binary Utilities, cuBLAS, and the CUTLASS docs ("Efficient GEMM in CUDA" and the CuTe tutorials).
- Hwu, Kirk and El Hajj, PMPP.
- Volkov, "Better Performance at Lower Occupancy".
- Williams, Waterman and Patterson, "Roofline".
- IEEE 754.
- Higham.
- The FP8 formats paper.
- AMD Composable Kernel.

Tier-1 material that was really read (H1 sources):

- `cuda_occupancy.h`
- `cuda_pipeline_primitives.h` and `cuda_pipeline_helpers.h` (cp.async mapping, sizes 4/8/16, maximum 8 stages)
- `crt/mma.h` (WMMA shapes per type, `__CUDA_ARCH__` guards, `num_elements`, `__float_to_tf32` via `cvt.rna.tf32.f32`)
- `cuda_fp16.h`, `cuda_bf16.h` and `cuda_fp8.h` conversions

## Decisions for the owner

1. **No GPU numbers.** None of the E6 or E7 speed tables were measured, so no time, GFLOP/s or "% of cuBLAS" appears anywhere. When a GPU is available, run the `sgemm_*` and `wmma_gemm` programs (build with `-arch=sm_XX -DUSE_CUBLAS -lcublas`) and add a real speed table to F6-21 to F6-26.
2. **The course card's "Low occupancy, high speed?" forensic (F6-20)** expects a profile where lower occupancy is faster. No profile can be produced without a GPU. F6-20 uses the register report, the ILP kernels' SASS and a *simulated* scheduler profile from its latency model (labelled as a stand-in for a profiler export). The scenario's "nearly twice as fast" is story text, not a measurement. Replace this with a real Nsight Compute comparison once one exists.
3. **The "Register spills" forensic (F6-23)** uses real compiler evidence: `__launch_bounds__(256,4)` forces 64 registers, giving 1,336 B of spill stores, 1,204 B of spill loads, and 301 LDL and 334 STL in the SASS, against 0 without it.
4. **Example device.** F6-20 defines an "example device" (cc 8.0-like limits) that F6-22, F6-23 and F6-27 reuse. It is a model and is labelled as such. Confirm that it is acceptable, or replace it with a real device's `deviceQuery` output.
5. **Analogy registry.** The chapters use only registered F6 mappings (hall, rows of helpers, pantry, table, tray machine for tensor cores). Proposed for registration:
   - "two trays": double buffering (F6-24).
   - "measuring cups with fewer or more marks": number formats (F6-25).
   - "the seating chart": layout (F6-27).
6. **The E7 lower-level version** (fragment layouts used directly, optional in the curriculum) is not built. It appears only as an optional lab step in F6-27.
7. **cuBLAS references** in the harnesses (`cublasSgemm` and `cublasGemmEx`) compile and link against CUDA 12.0, but their semantics are unverified. The chapters say so.
8. **F6-22's forensic evidence** is a deliberate deadlock. Its log shows `exit code: 124 (stopped by the 20 s time limit)`, which build.py accepts as a completed run.
