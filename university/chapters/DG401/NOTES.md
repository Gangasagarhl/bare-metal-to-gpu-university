# DG401 — Collectives, NCCL and RCCL: author notes

Six chapters, F8-01 to F8-06, level L4, 4 credits. Prerequisites: CU302 (or HP301 + CU302) and DS201.

- Labs are in `university/labs/F8-01` to `university/labs/F8-06`.
- Each lab passes `university/labs/run_lab.sh university/labs/<ID>` with exit status 0.
- `python3 university/build/build.py` reports no PROBLEM line for F8-01..F8-06, DG401, or any `#gl-` link they use.
- Glossary: `glossary.json` holds 34 new entries. Six terms are linked but not redefined, because other courses already define them with the same meaning:
  - Alpha-beta model, NVLink / NVSwitch, Infinity Fabric / xGMI, Peer-to-peer (P2P) (HW301);
  - Root complex, root port, switch, endpoint (HW204);
  - Data race (SP203, HW202).
- "Broadcast" is named "Broadcast (collective)". CU301 already uses "Broadcast" for the shared-memory read broadcast, and the two meanings should not be merged.

## What this build could and could not do

- **Tools available:**
  - g++ 13.3.0;
  - Open MPI 4.1.6, built with `-DOMPI_SKIP_MPICXX` and run with `--allow-run-as-root --oversubscribe`;
  - nvcc 12.0 and HIP 5.7 (gfx90a).
- **Not available:** a GPU, NCCL, RCCL, nccl-tests, rccl-tests, nvidia-smi, rocm-smi or amd-smi.
- **Substitutes used instead:**
  - Open MPI plays the real collective library.
  - Threads, or MPI processes on one 4-CPU container, play the GPUs.
  - TN-1 is an invented teaching node used by the models in F8-03 and F8-04. Its values are: GPU bridge 50 GB/s, PCIe 25 GB/s, CPU-to-CPU link 12.5 GB/s; the model uses α = 10 µs. These are labelled invented everywhere.
- **NCCL/RCCL API names and behaviour appear only in unverified boxes, never as listings.** This covers communicators, unique id, `ncclAllReduce`, group calls, stream semantics, `NCCL_DEBUG`, double binary tree, channels and topology detection.
- **Primary sources read:** only the installed headers.
  - H1: CUDA 12.0, `cuda_runtime_api.h` and `driver_types.h`, for the peer access functions, `cudaDeviceP2PAttr`, `cudaStreamWaitEvent` and `cudaMemcpyPeerAsync`.
  - H2: HIP 5.7, `hip_runtime_api.h`.
  - `ompi_info` output.
- **All other sources** (MPI Standard, Patarasuk and Yuan, Thakur, Rabenseifner and Gropp, NCCL and RCCL docs, nccl-tests notes, CUDA Programming Guide, PCIe specification, NVIDIA and AMD interconnect documents, the C++ standard, the PyTorch DDP paper) are cited as "Title only — not opened during this build (dossier gate G1 open)".
- **No hardware number of any real product appears.** Every timing comes from this container and is labelled as such.

## Listings and their status

| Chapter | Pass | Expected fail | Untested on hardware (compiled for real, exit 1 "no device") |
|---|---|---|---|
| F8-01 | `collectives.cpp`, `mpi_collectives.cc` (np 4) | `skip.cc`: a rank skips `MPI_Allreduce` and hangs, stopped by the time limit, exit 124 | — |
| F8-02 | `algorithms.cpp`, `rd_bug.cpp` (forensic, shows the wrong result), `allreduce_check.cc` (steps `ompi_algos` and `forced`, np 4 and 3) | — | — |
| F8-03 | `ab_model.cpp`, `pingpong.cc`, `allreduce_bench.cc`, `forensic` step | — | — |
| F8-04 | `topo_sim.cpp`, `busbw_curve.cpp`, `host_topo` step, `p2p_enums` step | — | `p2p_matrix.cu`, `p2p_probe_amd.hip` |
| F8-05 | `ring_allreduce.cpp` (ASan/UBSan, 4 B–16 MiB, N = 2, 4, 8), `ring_O2` step (1 GiB on N = 2 and 4) | `ring_race.cc` under ThreadSanitizer, exit 66 | `ring_gpu.cu`, `ring_gpu_amd.hip` |
| F8-06 | `dp_train.cpp`, `dp_forensic.cpp`, `dp_mpi.cc` (np 1, 2, 4) | — | — |

**Timing outputs from a shared container:**

- Which outputs: F8-02 `forced`, F8-03 `pingpong` and `allreduce_bench`, F8-05 `ring_O2`.
- They vary from run to run.
- The chapters quote the recorded values and say they are noisy.
- F8-02 and F8-03 were not re-run after their chapters were written. If they are re-run, check the quoted numbers in the chapters again: F8-02 text and table; F8-03 layer 2 and 3, worked example and answers.
- F8-05 quotes ranges ("about 0.7 to 2.7 GB/s"; "most of 20 runs") rather than single numbers.

**Lab Engineer's check runs (modified copies in scratch, not part of the recorded output):**

- F8-02: the N = 32 answer.
- F8-04: worked example (c) and answers 5 and 6.
- F8-05: answer 5.

The text says so wherever it quotes one.

## Unverified boxes, per chapter

- **F8-01:** NCCL/RCCL collectives (stream argument, asynchronous return, call-order rule, the set of collectives provided).
- **F8-02:** NCCL's ring and double binary tree and its per-size algorithm choice.
- **F8-03:** the nccl-tests busbw factors for collectives other than all-reduce.
- **F8-04** (6 boxes):
  - real link counts, rates and topologies, and PCIe peer-to-peer across root complexes;
  - `nvidia-smi topo -m`, `rocm-smi` and `amd-smi` notation;
  - NCCL topology detection, channels, `NCCL_DEBUG` and topology dump;
  - `p2p_matrix.cu` untested;
  - `p2p_probe_amd.hip` untested, including the meaning of HIP's performance rank;
  - the course forensic's missing real NCCL log.
- **F8-05** (3 boxes):
  - `ring_gpu.cu` untested, including peer access not enabled and per-tick event cost;
  - NCCL/RCCL fused kernels, channels, chunk/protocol choice and the nccl-tests busbw convention;
  - `ring_gpu_amd.hip` untested, including HIP's cross-device event wait semantics.
- **F8-06** (2 boxes):
  - the whole NCCL/RCCL communicator, stream, group-call, destroy/abort and `NCCL_DEBUG` model;
  - Open MPI's transport choice for Listing 2, and contention of library kernels with compute kernels.

## Decisions for the owner

1. **Course forensic "All-reduce at half speed" (F8-04).**
   - The card asks for a real NCCL debug output, provided. None exists in this build, and writing one from memory is forbidden (AH-17).
   - The evidence pack therefore uses the university's own ring planner output (`topo_sim`) and a model busbw curve (`busbw_curve`), both clearly labelled.
   - **Decision:** record a real `NCCL_DEBUG=INFO` (or RCCL) log and an nccl-tests curve on a two-socket machine, with the job placed across sockets, and add them as the "hardware" variant.
2. **TN-1 is invented.**
   - Keep it as the course's teaching node, or replace it with a measured lab machine once milestone F1 has been run there.
3. **No GPU run of any CUDA/HIP listing.**
   - Four listings are untested on hardware: `p2p_matrix.cu`, `p2p_probe_amd.hip`, `ring_gpu.cu`, `ring_gpu_amd.hip`.
   - They need a multi-GPU rerun of `run_lab.sh` before the course is released.
   - `ring_gpu.cu` should then also call `cudaDeviceEnablePeerAccess`; the chapter tells students to add it.
4. **Milestone F2 acceptance was done only on CPU threads.**
   - Sizes 4 B to 1 GiB were covered, but 1 GiB only for N = 2 and 4.
   - The "X % of NCCL/RCCL" target and the curve against `all_reduce_perf` need GPUs. X is left to the student's plan, as the curriculum says.
5. **Milestone F3 scaling efficiency** is not reported in this build. The chapter explains why: a tiny model on shared CPU cores.
6. **Exam P** ("predict all-reduce time from the model, then measure") is prepared by F8-03. Its prediction and measurement there use MPI on CPU processes; the GPU version needs hardware.
7. **Lab file rename.**
   - F8-05's GPU listings are named `ring_gpu.cu` and `ring_gpu_amd.hip`, not `ring_allreduce.cu`.
   - Reason: `run_lab.sh` names outputs by file stem, so `ring_allreduce.cu` overwrote the `.out`/`.log` of `ring_allreduce.cpp`.
   - The runner could warn about duplicate stems; that is outside this course's folders, so it was not changed.
