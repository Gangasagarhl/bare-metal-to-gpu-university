# CU301 CUDA II: parallel patterns — author's notes

Chapters F6-10 to F6-19 (L3), labs in `university/labs/F6-10` … `F6-19`, glossary in
`glossary.json` (60 four-part entries, generated from the chapters' jargon boxes; sources
"pending verification").

Build environment of every run: g++ 13.3.0, nvcc / cuobjdump / cu++filt from CUDA 12.0
(V12.0.140), hipcc 5.7 (gfx90a), CUB 2.0.1 (libcub-dev 2.0.1-2). **No GPU.** Every
`.cu`/`.hip` listing is built for real and its run records the runtime's own "no device"
error (exit code 1, `hardware: untested on hardware`). PTX/SASS/ptxas reports and CUB
header excerpts are produced by each lab's `run.sh` (compiled only / read only).

All ten labs were re-run at the end of this build with
`university/labs/run_lab.sh university/labs/F6-1x`: every run returned 0.
`python3 university/build/build.py` reports no PROBLEM line for these chapters.

## Global facts for the owner

- **No GPU number anywhere.** No time, bandwidth, speed-up or "% of CUB / % of copy" was
  measured; every such number in E3/E4/E5 is a learner measurement ("your X").
- **No document was opened.** CUDA C++ Programming Guide (D1), CUDA C++ Best Practices
  Guide (D2), Hwu/Kirk/El Hajj PMPP (D3), Nsight Compute and Compute Sanitizer docs,
  HIP docs, Harris "Optimizing Parallel Reduction in CUDA", Merrill and Garland
  "Single-pass Parallel Prefix Scan with Decoupled Look-back", Cormen et al.
  "Introduction to Algorithms" (F6-18 D4) are cited **title only** (dossier gate G1 open).
- **Tier-1 evidence that was really read:** installed CUDA 12.0 headers and CUB 2.0.1
  headers (quoted through `run.sh` steps so the quotes are reproducible).
- **Forensic labs without real evidence tools.** The course card's forensic "Bank conflicts
  in the transpose" expects a provided Nsight Compute output. None could be produced (no
  GPU) and inventing one is forbidden (AH-25); F6-11 uses kernel source + ptxas report +
  SASS address arithmetic + a CPU bank replay instead (unverified box in F6-11).
  **Decision for the owner:** replace with a real Nsight Compute report once a GPU run exists.
  "The reduction that is wrong only for large inputs" is in F6-14 (int index overflow,
  shown with PTX and a CPU arithmetic replay).
- **Analogy registry:** only registered F6 mappings are used (helpers, rows, table in the
  middle of the row, drawers = banks, tally counter = atomics, pantry). Small story
  extensions that are not new technical mappings: "tally sheet on each table" (privatization,
  F6-17), "board by the door" (tile status words, F6-16), "plates to the shelf by digit"
  (radix sort, F6-18), "shifting jars on the table / Rahim's rule" (padding / swizzle,
  F6-19). **Proposal for the Dean:** register "a private tally sheet per table" =
  privatization and "board by the door" = tile status / look-back.

## Per chapter

### F6-10 Shared memory
- Listings: `reuse.cpp`, `edges.cpp` (pass), `stencil.cu` (built; untested on hardware),
  `resources`, `sass_smem` (compiled only).
- Unverified: ptxas reports 1,056 bytes for a 1,048-byte array (rounding rule not
  documented here); stencil results on a GPU.

### F6-11 Bank conflicts
- Listings: `banks.cpp`, `replay.cpp` (pass); `strided.cu`, `transpose_bank.cu`
  (built; untested on hardware); `forensic_res`, `forensic_sass` (compiled only).
- Unverified: 32 banks × 4 bytes on every current NVIDIA GPU; 8/16-byte access phases;
  AMD LDS banks; Nsight Compute metric names; forensic evidence substitute (see above).

### F6-12 Barriers and the missing grid barrier
- Listings: `interleave.cpp`, `grid_barrier_sim.cpp` (pass); `grid_sync.cu` (built;
  untested on hardware); `ptx_bar`, `sass_gridsync` (compiled only).
- Unverified: the BPT.TRAP path for non-cooperative launches (SASS interpretation);
  memory-ordering guarantees of `grid.sync()`.

### F6-13 Atomics
- Listings: `lost.cpp`, `contention.cpp` (pass, deterministic seeded models);
  `atomics.cu` (built; untested on hardware); `ptx_atom`, `sass_atom` (compiled only).
- Unverified: automatic warp aggregation by ptxas (VOTEU.ANY + POPC + RED reading);
  where global atomics execute and their rates.

### F6-14 Reductions (milestone E3, exam P)
- Listings: `reduce_model.cpp`, `overflow_emu.cpp`, `sum_check.cpp` (pass; `sum_check`
  has a raised time limit); `reduce.cu`, `overflow.cu`, `cub_reduce.cu` (built; untested
  on hardware); `ptx_index`, `sass_shfl`, `cub_header` (compiled / read only).
- Unverified: no throughput, no speed-up between versions, no % of CUB.
- Decision for the owner: exam P's target percentage of CUB DeviceReduce must be set by
  the examiner before the attempt (the chapter says so; no number is suggested).

### F6-15 Warp-level primitives
- Listings: `warp_sim.cpp` (pass); `warp_prims.cu`, `warp_prims_amd.hip` (built; untested
  on hardware; the HIP run fails with hipErrorInvalidDevice); `ptx_warp`, `sass_warp`,
  `amd_wave` (compiled only, AMD ISA via `--save-temps`).
- Unverified: CUDA shuffle edge behaviour versus the HIP header rules; wave32/wave64 per
  AMD family.

### F6-16 Scans
- Listings: `scan_sim.cpp`, `device_scan.cpp` (threaded decoupled look-back on the CPU),
  `inplace_race.cpp` (pass); `scan.cu` (built; untested on hardware); `cub_lookback`
  (read only).
- Unverified: whether CUB 2.0.1 takes tile numbers from a counter; GPU memory-ordering
  primitives in CUB; forward-progress guarantee for started blocks.

### F6-17 Histograms and privatization (written in this build session)
- Listings: `privatize_model.cpp` (pass; 60 s limit), `hist_cases.cpp` (pass; E4 sizes and
  four input kinds, 36 cases, 0 failures), `stale_bins.cpp` (forensic replay, pass);
  `histogram.cu` (built; untested on hardware); `resources`, `sass_atoms` (compiled only),
  `cub_hist` (CUB header excerpt, read only).
- Unverified box: meaning of `ATOMS.POPC.INC.32` (compiler aggregation?) — check the
  instruction-set reference and time it on a GPU.
- Hedged in text: global bins "likely" stay in L2; Compute Sanitizer initcheck coverage of
  shared memory (asked the learner to check).
- Modelling choice: Listing 2's 320-block grid stands for "4 blocks × 80 SMs"; Listing 1
  uses 4 × the real SM count.

### F6-18 Radix sort (written in this build session)
- Listings: `split_demo.cpp`, `lsd_sort.cpp` (pass; E4 sizes × four kinds, keys and pairs,
  checked against `std::stable_sort`; signed and float keys), `unstable.cpp` (forensic
  replay, pass); `radix.cu` (built; untested on hardware); `sass_count`, `cub_kernels`,
  `cub_onesweep_atoms` (compiled only), `cub_radix`, `cub_policy` (read only).
- Unverified box: how CUB's onesweep path works (inferred from kernel names, policy flags
  and memory of the Adinets–Merrill method; dispatch logic not traced). The onesweep paper
  title must be confirmed by the Source Researcher (it is not in the guide's registry rows
  read in this build). CUB's throughput comments in `dispatch_radix_sort.cuh` were
  deliberately not reproduced.
- Interpretation flagged in text: the single `ATOMG.E.ADD` in the onesweep kernel "would
  fit" a dynamic tile counter.
- Difference documented: Listing 2 orders −0.0 before +0.0; CUB treats them as equal.
- Source D4 (Cormen et al.) is a textbook not named in the F6 registry rows; the owner may
  prefer PMPP's sorting chapter only.

### F6-19 Matrix transpose (written in this build session; milestone E5)
- Listings: `transpose_audit.cpp` (pass; segments and bank passes, CPU replay of every
  kernel on 1,000 × 700, 33 × 65, 1 × 77, 64 × 64: 0 errors), `nonsquare.cpp` (forensic
  replay, pass); `transpose.cu` (built; untested on hardware); `resources`, `sass_swizzle`
  (compiled only).
- Unverified boxes: the 32-bank 4-byte model for all current GPUs, 8-byte access phases,
  AMD LDS banks; the LOP3 truth-table convention (0xF0/0xCC/0xAA) used to read `0x3c` as XOR.
- The forensic lab of this chapter is the non-square indexing bug; the bank-conflict
  forensic of the course card lives in F6-11.
- E5's "% of E2 copy bandwidth": Listing 1 reports % of its own `copyTile`; the chapter
  tells the learner to use the E2 number in the report.

## Changes made while finishing this course (resumed build)

- Kept F6-10 … F6-16 and all lab sources from the interrupted run after validation; added an
  "explain in your own words" question (with answer) to F6-13, F6-14, F6-15, F6-16.
- Fixed glossary-link anchors in F6-10 … F6-13 and F6-19 so they match the generated
  `glossary.json` terms (Broadcast, Contention, Static and dynamic shared memory, Memory
  fence (__threadfence), Residency, Tile, Padding).
- Added `hist_cases.cpp` and the `cub_hist` step (F6-17); `cub_policy`, `cub_kernels`,
  `sass_count`, `cub_onesweep_atoms` steps (F6-18).
