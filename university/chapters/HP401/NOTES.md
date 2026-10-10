# HP401 HIP III: performance on AMD — author's notes

Chapters F7-15 to F7-20 (L4), labs in `university/labs/F7-15` … `F7-20`, glossary in
`glossary.json` (37 four-part entries whose slugs match every `gl-` link in the chapters;
sources "pending verification" unless they are this build's own runs).

Build environment of every run: g++ 13.3.0, hipcc HIP 5.7.1 (clang 17, targets gfx908,
gfx90a, gfx940, gfx942, gfx1030, gfx1100 as needed), clang-offload-bundler (LLVM 17),
nvcc / cuobjdump from CUDA 12.0, CUB 2.0.1, cuBLAS / cuBLASLt 12.0.2.224. **No GPU.**
Not installed: rocPRIM, hipCUB, rocWMMA, rocBLAS, hipBLASLt, Composable Kernel.

All six labs were re-run at the end with `university/labs/run_lab.sh university/labs/F7-xx`:
every run returned 0. `python3 university/build/build.py` reports no PROBLEM line for HP401.

## Global facts for the owner

- **No GPU number anywhere.** Every kernel run stopped at the runtime's "no device" error.
  The only timings are CPU-side teaching runs (F7-19), labelled "this machine, this run".
- **No document was opened.** AMD ISA guides (CDNA/CDNA2/CDNA3, RDNA3), HIP, rocPRIM,
  hipCUB, rocWMMA, rocBLAS, hipBLASLt and CK documentation, Merrill and Garland, the
  roofline paper and LLVM AMDGPU docs are cited **title only** (dossier gate G1 open).
- **Tier-1 evidence really read/produced:** HIP 5.7 headers (warpSize, `__ballot`
  return type, hipDeviceProp_t), CUB 2.0.1 and cuBLAS/cuBLASLt headers, compiler ISA and
  metadata for gfx90a/gfx908/gfx94x/gfx1100, SASS for sm_80.
- **AMD library APIs are in unverified boxes** (from author memory, AH-3): rocPRIM/hipCUB
  (F7-15), rocWMMA (F7-17), CK (F7-18), rocBLAS/hipBLASLt (F7-19). Their NVIDIA
  counterparts (CUB, mma.h, cuBLAS) were compiled as the verified stand-ins.

## Per chapter

### F7-15 Reductions and scans with rocPRIM / hipCUB
- Listings: block_scan.hip (untested on hardware), cub_device.cu (untested on hardware),
  tree_order.cpp (pass), lookback.cpp (pass); run.sh steps isa_scan (pass, compiled only),
  hip_nvidia (untested on hardware).
- Unverified boxes: rocPRIM/hipCUB interface; Listing 1 on hardware.
- Forensic: golden-value test broken by wave32 vs wave64 tree order (real CPU replay).

### F7-16 SGEMM with LDS tiling and register blocking
- Listings: sgemm_lds.hip (untested on hardware), tile_math.cpp (pass), tiled_replay.cpp
  (pass); steps resources, inner_loop, kai_isa (pass, compiled only), lds_limit
  (expected-fail: "local memory (65540) exceeds limit (65536)").
- Unverified boxes: no LDS/register-file sizes or bank counts from AMD documents; Listing 1
  on hardware.
- Forensic: missing second barrier (s_barrier 2 vs 1, plus replay).

### F7-17 Matrix cores: MFMA and rocWMMA
- Listings: mfma_gemm.hip (untested on hardware), fp16_error.cpp (pass); steps targets,
  isa_gemm, wmma_rdna3, lin_isa (pass, compiled only), cuda_wmma (untested on hardware).
- Unverified boxes: **MFMA 32x32x8f16 operand/result lane layout** (assumed; the listing's
  own check would catch an error on hardware); rocWMMA API; MFMA internal summation order;
  Listing 1 on hardware.
- Forensic: wrong `__gfx*__` guard leaves gfx942 on v_fma_mix_f32 ("Matrix cores unused").

### F7-18 Composable Kernel
- Listings: mini_ck.hpp + instances.cpp (pass), bad_instance.cpp (expected-fail,
  static_assert), mini_ck.hip (untested on hardware), nightly.cpp (pass; 4 of 8 shapes fail
  by design); step instances_isa (pass, compiled only).
- Unverified boxes: CK's structure and names (memory only); Listing 4 on hardware.
- Decision: the chapter teaches CK's ideas with a self-written mini library because CK is
  not installed. Replace or supplement with real CK once a ROCm machine is available.

### F7-19 rocBLAS / hipBLASLt and percentage of reference
- Listings: colmajor.cpp, harness.cpp, cold_ref.cpp, cold_ref_fixed.cpp (pass);
  cublas_ref.cu.inc (untested on hardware; built and linked); steps cublas_ver, harness_O2
  (pass).
- CPU numbers quoted in prose are from the final rerun (105.4 %, 52.3 %, 98 %; forensic
  "several hundred percent", 353 % in this run). They vary per run; re-sync if labs rerun.
- Unverified box: rocBLAS / hipBLASLt API.

### F7-20 Performance portability
- Listings: portable.hip (untested on hardware; also built through the NVIDIA path,
  untested), hazards.cpp (pass); steps macros, half_isa (pass, compiled only), fatbin
  (untested on hardware; bundle listing real), nv_ballot_v0 (expected-fail),
  nv_build (untested on hardware; SASS real).
- Unverified box: Listing 1 on hardware; `maxBlocks` values are placeholders.
- Finding worth knowing: HIP 5.7's NVIDIA-path `__ballot` fails for sm_80 (ptxas: vote
  without .sync); the listing wraps it with `__ballot_sync`.
- Finding: hipcc's host pass defines `__AMDGCN_WAVEFRONT_SIZE 64` even when a wave32 target
  is in the same binary; the chapter teaches run-time `prop.warpSize` for host decisions.

## Decisions for the owner

1. Confirm the MFMA fragment layout (F7-17) against the CDNA ISA guide before release.
2. All AMD library APIs (rocPRIM, hipCUB, rocWMMA, CK, rocBLAS, hipBLASLt) need a check
   on a ROCm install; the unverified boxes list exactly what to confirm.
3. The E9 table in F7-20 is a template with "not measured" entries; real rows need one AMD
   and one NVIDIA GPU.
4. Course forensic "Matrix cores unused" is implemented with real compiler evidence
   (wrong guard, gfx942); the profiler screenshot mentioned in the F7-20 scenario is
   described, not reproduced.
