# MP3 notes (handbook build)

## Files
- chapters/MP3/MP3.html, glossary.json (7 new terms), NOTES.md
- labs/MP3/: mp3_tolerance.hpp (L1), mp3_suite.cpp + mp3_mutants.cuh (L2), mp3_targets.hpp,
  targets_lock.cpp + targets_lock.in (L3), targets_example.txt, mp3_report.cpp (L4),
  mp3_report.in, forensic_final.in, synthetic_raw.py, mp3_gemm.cu (L5), run.sh, mp3_suite.timeout
- Reuses unchanged, by include: F6-21 cuda_shim.hpp and gemm_naive.cuh, F6-22 gemm_tiled.cuh,
  F6-23 gemm_regtile.cuh and launchers.cuh, F6-25 lowp.hpp.

## Listings and runs (run_lab.sh university/labs/MP3, exit 0)
- PASS: mp3_suite (R1: 6 correct cases 8/8 shapes, 3/3 seeded faults caught, ~33 s under ASan),
  targets_example (R4: READY, bda53db755989690), mp3_report (R5: 3 met, 2 not met, 0 invalid;
  synthetic data), synthetic_check (R11: byte-identical), env_label (R10), resources (R9: 124 regs,
  8192 B smem, no spills, compile only).
- EXPECTED FAIL: suite_square (R2: 1 of 3 faults caught), targets_lock on the template (R3),
  forensic_diff (R6), forensic_report (R7: fingerprint b14e10f24f5897f0, 1 invalid).
- UNTESTED ON HARDWARE: mp3_gemm, gemm_cublas (R8: version labels printed, then
  cudaErrorNoDevice). No GPU number appears anywhere; all times are synthetic and labelled.

## Unverified boxes
1. cuBLAS routine/settings for FP16-in/FP32-acc and whether it uses tensor cores; tensor-core
   internal accumulation order (M3).
2. Fused attention library (cuDNN or other): API, layout, masking (M4).
3. Clock-locking commands and options (Layer 3).
4. Profiler metric/section names; how cuBLAS selects kernels (M5).
5. FNV-1a name and constants in mp3_targets.hpp (from memory).
6. Listing 5 untested on hardware; cublasCreate/cublasSgemm semantics from CU302/CU401.
All D-sources are title only (dossier gate G1 open). D1/C1 are local project documents.

## Decisions for the owner
- Gates R0 and R3 are proposed (card requires R1, R2, R4); R2 placed after M2. Hold or drop?
- Rubric level descriptors (four levels) are this handbook's proposal; weights are the card's.
- The 10 % "noisy" spread threshold in Listing 4 is a handbook choice.
- Register the analogy "specialist station of the hall = vendor library" (F6-35 proposal) in the
  analogy registry, or replace it.
- Synthetic records for the report/forensic labs: acceptable as teaching data?
- Real-GPU runs of mp3_gemm (with and without cuBLAS) are needed before the handbook's GPU
  harness can be called tested.
- In the no-GPU container cudaDriverGetVersion reported 13000 (runtime 12000); the source of
  that driver library was not investigated.
- "Run fingerprint" (MP2 glossary) and "Targets fingerprint" (MP3) are related but distinct; check
  wording consistency across mega projects.
