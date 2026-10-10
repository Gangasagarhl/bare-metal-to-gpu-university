# MP4 The HIP port benchmarked against rocBLAS — author's notes

Handbook `MP4.html` (front matter `id: MP4`, `course: MP`, level L5), starter lab in
`university/labs/MP4`, glossary `glossary.json` (9 new four-part entries; every other term
is linked to its existing entry from HP301/HP302/HP401/CU302/MA302/SE402/SE501).

Build environment of every run: g++ 13.3.0, hipcc HIP 5.7.31921 (clang 17.0.6), nvcc and
cuobjdump from CUDA 12.0 (V12.0.140), Python 3.13.16. **No GPU, no ROCm libraries, no internet.**

`university/labs/run_lab.sh university/labs/MP4` returns 0 (re-run at the end).
`python3 university/build/build.py` reports no PROBLEM line that mentions MP4.

## Listings run

| Run | Result |
|---|---|
| `suite.cpp` (Listing 8; CPU emulator of F6-21 runs the real kernel text) | pass: 3 instances x 12 shapes, worst err/bound 0.986, seeded fault caught on 11 of 12 shapes |
| `forensic_stride.cpp` (Listing 13) | pass (evidence reproduced: only the 128-work-item instance fails) |
| `sgemm_mp4.hip` (Listing 9), lab runner build for gfx90a | built; **untested on hardware** (exit 1 at `hipGetDeviceProperties`, no device) |
| `fatbin` step: gfx90a + gfx942 + gfx1100 in one binary | bundle listing real; run **untested on hardware** |
| `nv_build` step: HIP NVIDIA path, sm_80 | build and SASS counts real; run **untested on hardware** |
| `isa_amd` step (Listing 10) | pass, compiled only |
| `hazard_scan` (F7-20's scanner over MP4 sources) | pass, 0 lines flagged |
| `mp3_sync` (Listing 14) | pass, in sync with MP3's shape matrix |
| `report_empty`, `report_fixture`, `report_lowered` (Listing 11) | pass; fixture numbers are invented and labelled FIXTURE |

No expected-fail listing in this lab.

## Real findings of this build worth knowing

- LDS bytes per instance from the arithmetic (8448, 6272, 6400) equal the compiler's
  `.group_segment_fixed_size` on gfx90a, gfx942 and gfx1100.
- gfx90a/gfx942 encode FP32 FMAs partly as `v_pk_fma_f32`; gfx1100 as `v_dual_fmac_f32` pairs.
- Instance i1 (128x64x8, 8x4) on gfx90a and gfx942: 160 FMA + 96 unfused multiplies per stage
  (`v_pk_mul_f32` + adds); gfx1100 and sm_80 fuse all 256. Cause not investigated (posed as a
  check question with hypotheses only).
- HIP 5.7's NVIDIA-path `hipGetDeviceProperties` does not fill `gcnArchName` (header read);
  the device key uses `major`/`minor` there.

## Dependencies on other folders (important for the owner)

- `mp4_check.hpp` includes `../MP3/mp3_tolerance.hpp` (MP3 Listing 1) unchanged.
- `shapes.hpp` copies MP3's `kShapes` from `../MP3/mp3_suite.cpp`; `mp3_sync.py` fails the lab
  if they drift (it takes the longest `kShapes` definition, because mp3_suite.cpp had an `#if`
  with a reduced set while MP3 was being written in parallel).
- Also reused: `../F6-21/cuda_shim.hpp`, `../F6-25/lowp.hpp`, `../F7-20/hazards.cpp`.
- MP3 was being written at the same time as MP4. If MP3 renames or moves these files,
  rerun this lab; the build will fail loudly, never silently.

## Unverified boxes (what to check, where)

1. Hardware, ROCm compatibility, versions, rocBLAS/hipBLASLt interfaces, profilers, clock
   setting, GPU address sanitizer (ROCm docs D2, D4, D5). No API of rocBLAS/hipBLASLt is named.
2. Rubric split of the 15 % (see decisions).
3. Hardware specifics (LDS capacity, banks, register files, wave slots, MFMA throughput): none
   given; ISA guides D3.
4. Listing 9 untested on hardware.
5. Format of `gcnArchName` with feature suffixes (the suite's probe `gfx90a:sramecc+:xnack-` is
   an assumption from memory; the header only says "AMD GCN Arch Name").

Sources D1–D6 are title only (dossier gate G1 open). Claims about MFMA availability per target
reuse HP401's real compile records (F7-17 `targets.log`, `wmma_rdna3.log`).

## Decisions for the owner

1. **Rubric split.** The card takes 15 % "from code quality and methodology" without saying how.
   This handbook splits it in proportion to MP3's weights: methodology 20 -> 10 %, code quality
   10 -> 5 % (final: correctness 30, performance 25, methodology 10, gaps 15, portability 15,
   code quality 5). Alternatives: 7.5/7.5, or all 15 from methodology.
2. **Optional gates.** The card requires R1, R2, R4. The handbook describes R0 (may be merged
   into R1) and R3 ("measurement freeze") as optional; the owner decides whether to require them.
3. **Calendar.** "About 12–16 weeks part-time" and the week plan of Figure 2 are a planning
   suggestion, not data.
4. **No AMD GPU available.** Whether M2–M3 may be passed with "untested on AMD hardware" marks
   (and a changed rubric) is the owner's decision at R0/R1.
5. **Analogy proposal** (needs registration by the Dean): "each building's master team = the
   vendor library" (F7-20 already used "each hall's master team" in prose; CU401 proposed
   "library = a specialist station"). The two proposals should be merged into one mapping.
6. **Gate signers.** Proposed: two independent reviewers at R1/R2 (a peer who passed HP401 and,
   where available, a mentor), the owner approving targets; a panel of two at R4.
7. **Targets digest vs MP3 fingerprint.** MP3 freezes targets with an FNV-1a fingerprint of a
   text file (targets_lock.cpp); MP4's `report.py` uses a SHA-256 digest of JSON because it
   handles two GPUs. A later revision could make MP4 reuse MP3's targets format per GPU.
