# HW301 — GPU hardware: SIMT machines: author notes

This course has nine chapters, F1-55 to F1-63, at level L3 and worth 3 credits. The prerequisites are HW202 and HW203.

- Labs are in `university/labs/F1-55` to `university/labs/F1-63`.
- Each lab passes `university/labs/run_lab.sh university/labs/<ID>` with exit status 0.
- All nine labs were re-run at the end of this build, on 2026-10-09. After that re-run:
  - the prose, figures and answer keys were checked against the `.out` files;
  - `python3 university/build/build.py` reports no PROBLEM line for F1-55..F1-63 or for any `#gl-` link used by them.
- A few quiz answers were checked by running modified copies of listings in scratch space. The text says so where it quotes one ("Lab Engineer's check run; not in the recorded output"):
  - F1-58: Q6, Q7 and the lab expectations;
  - F1-59: the worked example and Q7;
  - F1-60: Q7 (97.1 %);
  - F1-61: Q7;
  - F1-62: the worked example, Q7 and the forensic fix estimates;
  - F1-63: Q5 and Q7.

## Toolchain and local evidence (no GPU in this build)

- **g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0.** Every `.cpp` file is built with the runner's flags: `-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined`.
- **nvcc: Cuda compilation tools, release 12.0, V12.0.140.** cuobjdump comes from the same toolkit. The runner builds `.cu` files for its default target (sm_52). The extra steps in `run.sh` compile for explicit targets (sm_70 to sm_90) to show PTX and SASS.
- **hipcc, HIP 5.7.31921, driving Ubuntu clang version 17.0.6.** Builds use `--offload-arch=gfx90a`. Assembly is produced with `--cuda-device-only -S`. The wavefront-size macros come from `clang++-17 -x hip --cuda-device-only -nogpulib -nogpuinc -dM -E`.
- **Other tools.** rocminfo 5.7.1-3build1 (it reports "ROCk module is NOT loaded") and lspci 3.10.0 (a virtual machine with virtio devices only).
- **Headers actually read in this build.** These are the only primary sources that were opened:
  - **H1:** CUDA 12.0, Ubuntu package nvidia-cuda-dev 12.0.146~12.0.1-4build4. Read:
    - `driver_types.h`: the `cudaDeviceProp` comments;
    - `cuda_runtime_api.h`: the section "API synchronization behavior", the `cudaMallocHost` comment and the property descriptions;
    - `crt/mma.h`: the WMMA guard `__CUDA_ARCH__ >= 700`.
  - **H2:** HIP 5.7, package libamdhip64-dev 5.7.1-3. Read:
    - `hip_runtime_api.h`: the `hipDeviceProp_t` comments;
    - `amd_detail/amd_warp_functions.h`: `warpSize = __AMDGCN_WAVEFRONT_SIZE`.
- **No real hardware number appears anywhere.** No book, whitepaper, ISA guide or specification was opened. All D/J/S/W sources are cited as "Title only — not opened during this build; the Source Researcher must confirm the edition and section (dossier gate G1 open)". The chapters quote:
  - no SM or CU counts;
  - no bandwidths, link rates or clocks of real products;
  - no latencies of real products;
  - no bank counts presented as fact.

  Every number in the chapters comes from one of four places:
  - compiler or tool output from this build;
  - a header comment read in this build;
  - a measurement on the build container's CPU (F1-55 `chains`, F1-59 `host_bw`), labelled as such;
  - the invented teaching GPU TG-1, labelled "invented" wherever it is used.

## Listings run

**Nothing was run on a GPU.** Status key:

- **pass**: the run exits 0 with the expected output.
- **untested on hardware**: the build is real, and the run stops at its first runtime call with the runtime's own no-device error (exit code 1). That is the expected outcome in this build. Each such run is covered by an unverified box in the chapter.
- **compiled only**: the step reads generated code and needs no GPU (exit code 0).

| Chapter | Pass | Compiled only | Untested on hardware (exit 1, no device) |
|---|---|---|---|
| F1-55 | chains (measured on the container), little | stepper_sass | saxpy, stepper |
| F1-56 | waves, forensic, rocminfo (records "no GPU"), archs (nvcc target list) | — | device_query, device_query_amd |
| F1-57 | simt, reduce_sim (forensic) | ptx, sass, amd_isa, wavesize | warp_vote, warp_vote_amd |
| F1-58 | banks, occupancy | resources, sass_smem, amd_lds, forensic (ptxas reports A/B) | transpose, transpose_amd, pressure |
| F1-59 | peak_bw, host_bw (measured on the container), forensic | — | mem_props |
| F1-60 | latency_sim, forensic | sass_loads, amd_wait | fma3, fma3_amd |
| F1-61 | precision | ptx_wmma, sass_wmma, hmma_gens, amd_mfma, amd_targets (gfx1030/gfx1100 rejection recorded), forensic | wmma_tile, mfma_amd, gemm_plain |
| F1-62 | copy_model, forensic | pci_tree (lspci) | peer |
| F1-63 | passport_check, forensic | evidence | probe, probe_amd |

None of these is an expected failure in the build's sense: every `run_lab.sh` call exits 0.

The course lab "deviceQuery / rocminfo on the lab GPU (GPU required; recorded output)" **could not be done**. The F1-56 and F1-63 chapters say so and leave the passport's device-query fields open. Lab staff must record the real output on the lab GPU.

## Unverified boxes, by chapter

- **F1-55**
  - Listing 3 (saxpy) was not tested on hardware.
  - The memory-latency scale is left to the GPU's documentation.
- **F1-56**
  - SM partitioning into sub-cores and schedulers.
  - "4 SIMD units per CU" on CDNA.
  - RDNA WGP (workgroup processor) details.
  - Device-query programs not tested on hardware (plus a GPU-required warning box).
- **F1-57**
  - Independent thread scheduling, `*_sync` mask rules and convergence-barrier SASS instructions (from memory).
  - Listings 2 and 3 not tested on hardware.
- **F1-58**
  - Why ptxas reported "Used 24 registers" with `-maxrregcount=16` (not explained by the output).
  - Bank count and width on NVIDIA and AMD; L1 and shared memory unified on Volta and later; LDS size on gfx90a.
  - Listings 1 and 2 not tested on hardware.
- **F1-59**
  - How many transfers there are per reported memory clock (and the "×2" bandwidthTest convention).
  - HBM channel and pseudo-channel organisation, interposer and TSVs, GDDR6 channels per chip, data rates per generation.
  - Listing 3 not tested on hardware, and whether CUDA 12.0 still fills in the "Deprecated" `memoryClockRate`.
- **F1-60**
  - How NVIDIA tracks dependences (control codes and dependency barriers versus a scoreboard).
  - Number of schedulers and issue width per SM or CU.
  - Listings 2 and 3 not tested on hardware.
- **F1-61**
  - `s_nop` wait-state semantics and MFMA wait states.
  - The AGPR/`accum_offset` register split on gfx90a.
  - Per-quad-pair operation of `HMMA.884`.
  - Reading SASS HMMA names as m×n×k. This reading makes all the counts consistent, but it is an interpretation.
  - Which formats each generation supports.
  - Lane-to-element layout of the MFMA.
  - Listings 1 and 2 not tested on hardware.
- **F1-62**
  - All link rates and link counts (none are given).
  - NVSwitch equidistance, xGMI topologies, platform rules for PCIe P2P.
  - Listing 2 not tested on hardware; the HIP equivalents were not built.
- **F1-63**
  - The typical structure of a whitepaper: peak-table conditions (boost clock, sparsity) and per-package versus per-die counts.
  - Probe listings not tested on hardware; the device-query step was not done.

## Decisions for the owner

1. **Named product (F1-63; course card "one named product").**
   - Proposal: gfx90a / AMD Instinct MI200 series. Documents: the "AMD CDNA 2 Architecture" whitepaper and the "AMD Instinct MI200" ISA reference guide.
   - Alternative: an sm_80 product with the "NVIDIA A100 Tensor Core GPU Architecture" whitepaper.
   - All titles were written from memory and need gate G1.
   - The choice should match the lab GPU that is actually available.
   - Until the owner decides, the chapters say "the named product" and quote no product numbers. The F1-63 decision box states this.
2. **TG-1, an invented teaching GPU.**
   - Used for every worked example and model.
   - Values: 4 SMs, warp of 32; per SM: 16 warps, 4 blocks, 16,384 registers, 32 KiB shared memory; model latencies of 100 and 4 cycles; 256-bit memory at 8 GT/s; host link of 16 GB/s with 10 µs per copy.
   - Labelled "invented" everywhere.
   - Proposal: register TG-1 in the guide as a shared teaching device, so that other GPU courses reuse the same numbers.
3. **Analogy.**
   - The chapters use F1's restaurant world.
   - The GPU is the "banquet hall", which reuses the registered mappings of F6's great kitchen hall: section = SM/CU, row of helpers = warp/wavefront, supervisor = scheduler, tray = registers, side table = shared memory/LDS, warehouse = device memory, corridor = PCIe.
   - Proposal: add "banquet hall" to the analogy registry as F1's name for the F6 mapping, or rename it to "great kitchen hall" in these chapters if the owner prefers one name.
4. **Forensic placeholders X1–X5 (F1-63).**
   - The colleague's passport in the forensic lab has its hardware numbers replaced by X1–X5, so that no unverified number is printed.
   - The checker's rules are unaffected.
   - The owner may replace them with tagged numbers once the named product's documents pass G1.
5. **No-device outputs are kept as evidence.**
   - Runs on CUDA and HIP print the runtime's own error and are recorded with exit code 1, not hidden: `cudaErrorNoDevice`, and `hipErrorInvalidDevice` as printed by HIP 5.7.
   - The builder accepts them (each `.log` has "exit code:").
   - On a GPU machine they should be re-run and their expected outputs confirmed. Each chapter's unverified box gives the expected line.
6. **WMMA listing target.**
   - `wmma_tile.cu` is guarded with `__CUDA_ARCH__ < 700 → __trap()` because the runner builds for sm_52.
   - The chapter tells students to add `-arch=sm_80` to run it on a GPU.
   - The owner may prefer that `run_lab.sh` accept a per-lab architecture.
7. **Measured CPU numbers.**
   - F1-55 (`chains`) and F1-59 (`host_bw`) are real measurements of the build container and change on every re-run.
   - The prose quotes only rounded or qualitative values ("about 1.9 ns", "ratios close to 4", "read the median from the output").
   - The exact numbers are in the inserted `.out` files.
8. **Course forensic "Why is my GPU idle?"** This is the F1-62 forensic lab: a copy/compute model of the recorded job, 3.6 % GPU busy, explained from the link, the DMA engines and pageable memory.
9. **Curriculum seed glossary terms.** `glossary.json` uses the seed's exact term names, so our four-part entries take the seed ids: SM, Compute Unit (CU), Warp / wavefront, LDS, HBM, Occupancy, Tensor Core / Matrix Core, MFMA / WMMA, NVLink / NVSwitch, Infinity Fabric / xGMI, PTX / SASS, Bank conflict, CDNA / RDNA.
   - Five terms also exist in HW202 or HW203 and are merged by the builder: Latency, Throughput, Instruction-level parallelism (ILP), Register file, Little's law. DRAM also appears in HW203.

## Sources to confirm (gate G1)

- **Books.**
  - Hwu, Kirk and El Hajj, "Programming Massively Parallel Processors".
  - Hennessy and Patterson, "Computer Architecture: A Quantitative Approach".
  - Little, "A Proof for the Queuing Formula: L = λW".
- **NVIDIA documents.** "CUDA C++ Programming Guide", "Parallel Thread Execution ISA", "CUDA Binary Utilities", and the NVLink/NVSwitch documentation.
- **AMD documents.** "AMD Instinct MI200" ISA reference guide (CDNA2), "AMD CDNA 2 Architecture" whitepaper, and the HIP documentation.
- **LLVM.** "User Guide for AMDGPU Backend".
- **Standards.**
  - JEDEC HBM and GDDR6 standards (document numbers to be recorded).
  - PCI-SIG "PCI Express Base Specification".
  - IEEE 754. bfloat16 needs a separate source.
- **Alternative named-product document.** "NVIDIA A100 Tensor Core GPU Architecture" whitepaper.
