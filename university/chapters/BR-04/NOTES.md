# BR-04 — From one GPU to many GPUs to many machines: author notes

Bridge chapter, level L4, placed before F8-01 (guide 6.2). Prerequisites written in the front matter:
CU302; CU303 (F6-28, F6-29, F6-33); F6-06; F6-09; F1-62; DS201 (F5-07); F5-22; F6-37 recommended.
F8-01 already lists BR-04 as a prerequisite, and F6-37 already calls itself "the first half of bridge
BR-04" and points to "bridge BR-04's lab". This chapter is written to match both.

- Lab: `university/labs/BR-04`. `university/labs/run_lab.sh university/labs/BR-04` exits 0 (about 50 s).
- `python3 university/build/build.py` prints no PROBLEM line that mentions BR-04 or its glossary anchors.
- Fragment checks done: html.parser balance (no errors), 67 ids all prefixed `BR-04`, no duplicates,
  no URLs, no script or event handlers. All 21 sections plus the answers section and the forensic answer key.
- Prose length is about 9,100 words without code, tables and SVG. That is the top of the L4 range. The
  extra length comes from treating "Carries over", "Changes" and "Traps" in depth, each with a real run.

## Listings run (this build: g++ 13.3.0, Open MPI 4.1.6, nvcc 12.0; 4-CPU container shared with other jobs; no GPU)

| Run | Listing | Result |
|---|---|---|
| link_roofline | Listing 1 (`link_roofline.cpp` + `.in`), invented links | pass, exit 0 |
| ring_order | Listing 2 (`ring_order.cpp`), invented TN-4/TN-8 | pass, exit 0 |
| peer_matrix_shm, peer_matrix_shm2 | Listing 3, 4 ranks, shared memory, run twice | pass, exit 0 (measured) |
| peer_matrix_tcp | Listing 3, 4 ranks, TCP over loopback | pass, exit 0 (measured) |
| roofline_measured | Listing 1 fed with the two measured fits | pass, exit 0 |
| ring_shm, ring_tcp | Listing 4 (prediction printed first, then the run; results checked against MPI_Allreduce: 0 differences) | pass, exit 0 (measured) |
| mismatch_ready | Listing 5, rank-dependent bucket order | expected-fail: hang after two ranks aborted with heap corruption; exit 124 under the 10 s limit |
| mismatch_fixed | Listing 5, agreed order | pass, exit 0 |
| fail_one | Listing 6, rank 2 SIGKILLs itself | expected-fail: mpirun stops the job, exit 137 |
| step_model | Listing 7 (`step_model.cpp`), invented values | pass, exit 0 |
| two_gpus | Listing 8 (`two_gpus.cu`) | built for real; run stops at `cudaGetDeviceCount` with `cudaErrorNoDevice`, exit 1 — untested on hardware |
| cuda_api | grep of `/usr/include/cuda_runtime_api.h` for the calls of Listing 8 | exit 0 |
| errno14 | Python `errno` / `os.strerror(14)` | exit 0 (`EFAULT Bad address`) |
| ompi_yield | `ompi_info` description of `mpi_yield_when_idle` | exit 0. This step was added to run.sh after the last full lab run, and its commands were executed by hand in the same format. The next full run regenerates it. |

**Important for whoever re-runs the lab.** The chapter text quotes numbers from the recorded `.out` files
of 2026-10-10. These include the fits (shared memory: α 0.41 µs, β 8.573 GB/s; second run: β 3.745;
TCP: α 74.67 µs as an outlier, β 2.285), the measured/predicted ratios, and the forensic-lab numbers.
These are real measurements on a shared machine. **A re-run will change them.** If the lab is re-run, re-read
Layer 3 (trap 1 and trap 2), the lab's "Expected observations", the forensic lab and its key, and
update every quoted number. The forensic lab in particular depends on the 8-byte TCP outlier of this run. If a re-run
does not reproduce an outlier, keep the old `.out`/`.log` pair under a new name (for example
`peer_matrix_tcp_2026-10-10`) and point the forensic lab at it. Do not hand-edit numbers.

Run notes:
- Every MPI run uses `--mca mpi_yield_when_idle 1`. During development, runs without it showed one-way times
  of 2–6 ms for 8-byte messages and a negative fitted β. The cause is busy-polling processes losing their CPU on the
  shared container. Those development runs were not kept as logs. The chapter says so.
- The β fit in Listing 3 was changed from "slope of the two largest sizes" to "largest size / (time − α)",
  because the slope version produced a negative β on a noisy run (see Troubleshooting).
- `mismatch_ready` is non-deterministic in detail: which lines appear and in what order varies. The first
  development run also hung, and also printed the "Read -1, expected 500000, errno = 14" and heap-corruption
  lines. The chapter quotes only what the recorded run shows.

## Claims not verified (unverified boxes and title-only sources)

1. **Unverified box (Layer 3, failure).** How NCCL/RCCL behave when a rank dies (an error, a timeout or a
   hang), communicator abort, and watchdogs. To check: the NCCL documentation (asynchronous error handling,
   communicator abort) and the RCCL documentation.
2. **Unverified box (Hardware).** Untested on hardware. None of the following was observed: GPU peer, NVLink, xGMI,
   network or RDMA transfers. The description of switches, root complexes and GPU-to-GPU links follows the documents named in curriculum
   Track F, which were not opened. No real link speed is stated anywhere.
3. **Inference in trap 3.** The reading of "expected 500000" (half of the 1,000,000-byte bucket, moved into or out
   of a 4,000-byte buffer) is presented as a plausible inference from the numbers. It was not verified inside Open MPI.
4. **Sources D1–D10** are all "title only, not opened" (dossier gate G1 open): the CUDA Programming Guide, the
   MPI Standard, Patarasuk and Yuan, Thakur/Rabenseifner/Gropp, the nccl-tests performance notes, the PCIe spec and
   NVLink documents, the AMD Instinct/ROCm topology documents, the NCCL/RCCL documentation, Williams/Waterman/Patterson
   (Roofline; the guide's registry row for "Papers of curriculum items 75–85" covers it as curriculum item 75),
   and Li et al. (PyTorch DDP).
5. "Communication libraries take a stream argument" (Layer 2) is tagged D8 (title only). F8-01 has the matching
   unverified box.

## Invented values (labelled invented in code, output and text)

- TN-4 (from F6-37): same switch α 6 µs, β 12 GB/s; root complex α 9 µs, β 9 GB/s; host link α 10 µs, β 16 GB/s.
- TN-8 (new, this chapter): inside an island α 5 µs, β 40 GB/s; crossing path α 10 µs, β 10 GB/s, shared per direction.
- Links X (α 2 µs, β 4 GB/s) and Y (α 20 µs, β 40 GB/s).
- Step model: T1 800 ms, gradient 256 MiB, a network with α 20 µs and β 5 GB/s.
- The "shared crossing path divides β by the number of hops" rule is a stated modelling assumption of Listing 2.
  It is not a measured property.

## Decisions for the owner

1. **New analogy details.** These stay within the registered F8 mapping ("chain of kitchen halls with a
   delivery service"; "passing bowls around a round table"). They are: "corridors in one building vs roads across town" for
   scale-up/scale-out, and "soup vs salad at the same table" for a collective mismatch. **Please register them, or
   ask for a rewording.**
2. **Glossary.** Four new terms: Scale-up and scale-out (GPU communication), Link roofline, Message-size sweep,
   Collective mismatch. **"Scale-up/scale-out" is this university's usage, defined in the chapter. It is not quoted
   from a source.** All other terms are linked to the existing entries: Alpha–beta model, Topology, Topology (of a node), P2P,
   PCIe, NVLink/NVSwitch, xGMI, Stream/Event (CUDA), Roofline model, Ridge point, Half-performance size,
   Collective operation, All-reduce, Ring all-reduce, Ring order, busbw, Ping-pong, Communication-computation
   overlap, and Watchdog.
3. **Shared-memory and TCP-loopback MPI processes stand in for GPUs and for a network** in the lab's measured
   part. The chapter says clearly that their numbers say nothing about GPU links. The owner may prefer that this
   part be marked more prominently as a substitute.
4. **Overlap with F6-37 and F8-03.** The peer matrix and the predict-then-measure step also appear in those chapters.
   BR-04 keeps them as its lab, as the bridge card asks. They are used here to teach the three traps, and they link
   forward rather than repeat the deeper material.
5. **Prerequisites.** BR-04's front matter names CU303 chapters and F6-37 "recommended". DG401's card requires
   only CU302 and DS201, so a learner may arrive without CU303. The owner may want CU303 added to DG401's
   prerequisites, or BR-04's list relaxed.
