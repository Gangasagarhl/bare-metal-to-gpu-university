# DG402 — MPI, RDMA and multi-node GPU: author notes

Five chapters, F8-07 to F8-11, level L4, 4 credits. Prerequisites: DG401, DS401.

- Labs are in `university/labs/F8-07` to `university/labs/F8-11`.
- Each lab passes `university/labs/run_lab.sh university/labs/<ID>` with exit status 0.
- `python3 university/build/build.py` should report no PROBLEM line for F8-07..F8-11 or DG402.
- Every chapter has:
  - the 21 template sections, plus "Answers to Check yourself" with a forensic answer key;
  - two inline SVG figures;
  - a jargon box and a Transition box;
  - claim tags on its sources;
  - unverified boxes, and untested-on-hardware boxes where they apply.
- Approximate word counts (validator): F8-07 7,200; F8-08 7,650; F8-09 7,050; F8-10 6,800; F8-11 6,800. The brief asks for about 5,000. The overrun comes mostly from the line-by-line tables and the evidence packs. Decision for the owner: trim or keep.

## Glossary

`glossary.json` holds 23 new entries, generated from the chapters' jargon boxes and in the same four-part format: simple, analogy, precise, plus source, related and chapters. Each slug matches the `#gl-` links the chapters use.

**Linked but not redefined** (another course already defines the term with the same meaning):

- Rank (MPI) — DS303.
- RDMA (remote direct memory access), GPUDirect RDMA, Queue pair (QP) — HW205.
- Verbs, perftest — DS401.
- Alpha–beta model, Pinned (page-locked) memory — HW301.
- Collective operation, All-reduce, Reduce-scatter, All-gather, Ring all-reduce, Bus bandwidth (busbw), Gradient bucketing, Data-parallel training — DG401.
- CUDA event — CU201.
- Stream (CUDA), Overlap efficiency — CU303.

F8-09's jargon box still explains RDMA, GPUDirect RDMA and perftest for the reader, but `glossary.json` does not repeat them.

**Naming decisions:**

- "Communicator (MPI)" is kept next to DG401's "Communicator", because the MPI object has MPI-specific semantics such as split and handles. Decision for the owner: merge or keep.
- "Gradient bucket" was planned but dropped in favour of DG401's "Gradient bucketing".
- F8-07's link to the old `gl-collective` slug was changed to `gl-collective-operation`, because DG401 renamed the term.

## Environment of this build

**Tools present:**

- Open MPI 4.1.6, Debian package, MPI API 3.1.0.
  - Built without CUDA support: `MPIX_CUDA_AWARE_SUPPORT` 0, and `ompi_info` shows no CUDA option in the configure line.
  - Configured with `--with-ucx --with-verbs --with-libfabric --with-psm2`.
- UCX 1.16.0, with modules cma, ib, rdmacm and rocm perftest. There is no CUDA module.
- libibverbs 50.0-2ubuntu0.2.
- g++ 13.3.0.
- nvcc 12.0.

**Machine:** one cloud container with 4 logical CPUs shared with other jobs.

**Not available:**

- a GPU;
- an RDMA device: `ibv_get_device_list` fails with errno 38, and neither `/sys/class/infiniband` nor `/dev/infiniband` exists;
- a second machine;
- perftest (`ib_write_bw`), `ibv_devinfo`, nvidia-smi;
- NCCL, RCCL, nccl-tests, rccl-tests;
- the OSU Micro-Benchmarks;
- a profiler.

**Substitutes used instead:**

- several MPI processes on one machine;
- TCP forced with `--mca btl tcp,self` as the "network" (F8-09);
- simulated nodes made by splitting ranks (F8-10);
- CPU stand-ins for the backward pass (F8-11);
- alpha-beta models that use invented parameters.

**Invented teaching values (TN-2):** they describe no real product, and every output that uses them prints "invented".

| Where | Parameters |
|---|---|
| F8-09 `two_node_model.in` | copy α 8 µs at 20 GB/s; network α 4 µs at 20 GB/s; NIC reading GPU memory at 20 GB/s; chunk 512 KiB |
| F8-10 `hier_model.in` | intra-node α 2 µs at 80 GB/s; inter-node α 4 µs at 20 GB/s per node, one NIC per node |
| F8-11 `timeline_model.in` | α 30 µs, 10 GB/s; eight layers with invented backward times and gradient sizes (64 MiB for layer 0) |

Decision for the owner: accept the name "TN-2", or align it with DG401's "TN-1" scheme.

**Primary sources read in this build** (the installed headers and tool outputs only):

- `mpi.h` and `mpiext_cuda_c.h` from Open MPI;
- the help file `help-mpi-runtime.txt`;
- the output of `ompi_info`;
- CUDA 12.0 `driver_types.h`, `cuda.h` and `cuda_runtime_api.h`;
- the libibverbs headers, through the compiler.

Every other source is listed as "Title only — not opened during this build (dossier gate G1 open)":

- the MPI Standard;
- Open MPI, UCX, OSU, NCCL and RCCL documentation;
- the InfiniBand specification;
- the rdma-core and perftest documentation;
- NVIDIA's GPUDirect RDMA documentation and the ROCm documentation;
- Thakur, Rabenseifner and Gropp; Patarasuk and Yuan;
- the HAN paper (title not given; to be found by the Source Researcher);
- the PyTorch DDP paper (S. Li et al.);
- the Nsight Systems and ROCm Systems Profiler guides.

## Listings and their status

| Chapter | Pass (exit 0) | Expected fail | Untested on hardware (real build, runtime's own error) |
|---|---|---|---|
| F8-07 | `mpi_basics.cc` (np 1, 2, 4); `halo1d.cc` (np 1, 2, 4; hash `9fef6c9047951684` on all); `eager_limits` (`ompi_info`); `headtohead.cc` fixed version (`h2h_fixed`) | `headtohead.cc` naive version under vader (`h2h_vader`) and under tcp (`h2h_tcp`): hangs at 4096 and 65536 bytes, stopped by the time limit, exit 124 | — |
| F8-08 | `aware_probe.cc` (compile time 0, run time 0, decision "stage"); `ompi_build` (`ompi_info` and the UCX modules); `pingpong_staging.cc` | `force_cuda`: `--mca mpi_cuda_support 1` on a library without CUDA prints the "[no cuda support]" help text, then a segfault, exit 139 | `halo_device.cc` (CUDA and MPI): prints "halo path: staged", then `cudaErrorNoDevice`, exit 1 |
| F8-09 | `pingpong_net.cc` over shared memory and over TCP; `transport_shm` and `transport_tcp` (verbose BTL logs); `environment`; `two_node_model.cpp` | `rdma_devices.cc` (libibverbs): `ibv_get_device_list` errno 38, exit 2 ("no device", expected) | `gdr_probe.cu`: `cudaErrorNoDevice`, exit 1. `rdma_devices.cc` is also untested on RDMA hardware |
| F8-10 | `hier_allreduce.cc` checked (np 4/ppn 2, np 8/ppn 4, np 8/ppn 2, np 6/ppn 4 unequal); `hier_model.cpp`; `han_info` (`ompi_info`) | `hier_allreduce.cc nocheck` on np 6/ppn 4 (`hier_bug`): the program exits 0 and prints FAIL lines, which are the forensic evidence | — |
| F8-11 | `overlap_mpi.cc` (serial, overlap and notest schedules, np 2); `timeline_model.cpp` | — | `overlap_streams.cc` (CUDA and MPI): `cudaErrorNoDevice`, then `MPI_ABORT`, exit 1 |

**Build conventions:**

- MPI sources and CUDA-with-MPI sources use the `.cc` extension, so that `run_lab.sh` does not try to build them on its own. Each lab's `run.sh` builds them.
- CUDA-with-MPI sources are built as `nvcc -x cu … $(mpicxx --showme:compile) … -lmpi`.

Decision for the owner: keep the `.cc` convention, or teach `run_lab.sh` about MPI.

**Do not rerun the labs without updating the quoted numbers.** These outputs are timings from a shared container and change on every run. The chapters quote them to two decimals.

- **F8-07 and F8-08:** the eager limits are stable. `pingpong_staging`'s ratios are quoted in F8-08 Layer 2, the worked example and the forensic lab.
- **F8-09:** the `pingpong_shm` and `pingpong_tcp` rows and fits (α 0.27 and 8.19 µs; β 5.31 and 2.42 GB/s). They are quoted in Layer 2, the Listing 1 table, Lab, Check yourself 7 and the forensic key.
- **F8-11:** every number in `overlap_mpi.out`. They are quoted in Layer 2, Layer 3, the worked example, the summary, the answers, the exam marking points and the forensic key. The trace shapes are robust; the milliseconds are not.
- **Deterministic outputs** (all model outputs, `hier_ok`, `hier_bug`, `halo1d`'s hash and values, `ompi_info` outputs): these can be rerun safely.

**Scratch check (not recorded, not quoted):** a timing of the `MPI_Iallreduce` start call in `/tmp/claude-0/work/DG402/post.cc`. It was used only to word F8-11's unverified box carefully: the start calls are not free. The chapter instead tells the learner to measure them (Lab step 2).

## Unverified boxes, per chapter

- **F8-07:**
  - the MPI Standard's semantics: send modes, unsafe programs, non-overtaking, `MPI_PROC_NULL`, `MPI_Sendrecv`, the default error handler;
  - Open MPI's eager and rendezvous protocol description;
  - the vader shared-memory copy mechanisms. Only the "cma" value was observed.
- **F8-08:**
  - the `--with-cuda` configure option;
  - the UCX CUDA module name (`libuct_cuda`-like);
  - how device pointers are detected inside the library;
  - the ROCm-aware equivalents;
  - the OSU device-buffer options.

  Untested on hardware: `halo_device.cc`. Its expected hash equality with F8-07 has not been observed.
- **F8-09:**
  - the GPUDirect RDMA requirement chain: GPU families, drivers, the kernel peer-memory component and its alternatives;
  - the root complex being slower or restricted for PCIe P2P;
  - Open MPI 4.1 preferring UCX on InfiniBand;
  - perftest's GPU-buffer options;
  - the NCCL/RCCL switches for GPUDirect RDMA on and off. None is named, on purpose;
  - the ROCm equivalents.

  Untested on hardware: `rdma_devices.cc` (the device and port branches) and `gdr_probe.cu`.
- **F8-10:**
  - what `coll_han_priority` 0 implies for selection;
  - HAN's internal algorithms;
  - NCCL/RCCL ring and tree selection across nodes;
  - rail-aligned networks;
  - the launcher's mapping options.
- **F8-11:**
  - which coll component carried `MPI_Iallreduce`;
  - whether this Open MPI build has an asynchronous progress thread;
  - how the backward-pass growth splits between Testsome, the start calls and interference;
  - NCCL/RCCL collectives being GPU kernels that use compute resources.

  Untested on hardware: `overlap_streams.cc`.

## Curriculum mapping

- **Milestone F4:** F8-07 and F8-08. The stencil gives the same answer on 1, 2 and 4 ranks, as observed. The OSU device-buffer runs need a GPU-aware build and a GPU, so they were not run.
- **Milestone F5:** F8-09 and F8-10. All three acceptance tests are quoted verbatim. None can be evaluated without two RDMA nodes.
- **Milestone F6:** F8-11. The acceptance test is quoted verbatim. The CPU stand-in shows why "the saving is close to the hidden communication time" is the right test: about 7 ms were hidden, and the backward pass grew by about 7 ms.
- **Course forensic "Staging through host memory":** F8-08, evidence E1–E5.
- **Exam P "explain a two-node timeline":** a practice item with marking points at the end of F8-11.
- **MP5:**
  - The brief for the multi-node part is in F8-11, with the rubric at 30/25/25/20.
  - F8-10 has the hierarchical-milestone part.
  - Fault handling (F8) is deferred to DG403.

## Analogy mappings (proposals for the analogy registry)

The F8 world is "a chain of kitchen halls with a delivery service between them". The new mappings proposed by this course are:

- **Rank** (F8-07): the hall's number on the delivery list.
- **Message** (F8-07): a labelled parcel.
- **Communicator** (F8-07): the delivery list a parcel is addressed within.
- **Eager and rendezvous** (F8-07): small parcels are left at the door; large parcels wait until the receiver opens the door.
- **Halo** (F8-07): tasting the edge plate of the neighbouring hall's stretch of table. This one is used in the jargon box but is not in the meta line.
- **Host staging** (F8-08): carrying crates from the hall's pantry out to the loading dock before the van can take them.
- **GPU-aware MPI** (F8-08): a delivery crew allowed to recognise pantry crates and fetch them itself.
- **Second node** (F8-09): a hall in another building across town.
- **Network** (F8-09): the road between buildings.
- **GPUDirect RDMA** (F8-09): the courier with a key to a shelf inside the pantry. This extends DS401's mapping of the courier with the key to one labelled shelf.
- **Pipelined staging** (F8-09): a bucket chain carrying crates in batches.
- **Node** (F8-10): a building.
- **Local rank** (F8-10): the station number of a hall inside its building.
- **Shard** (F8-10): one station's share.
- **Inter-node phase** (F8-10): station j of each building phoning only station j elsewhere.
- **Gradient bucket** (F8-11): a crate packed as soon as one course is finished.
- **Overlap** (F8-11): the van leaving while the kitchen cooks the next course.
- **MPI progress** (F8-11): someone must go out and wave the van on.
- **Exposed communication** (F8-11): the kitchen idle while it waits for the last van.

## Other decisions for the owner

1. **Untested GPU and RDMA steps:**
   - every lab step on GPUs, RDMA networks, two nodes, OSU, perftest, nccl-tests or rccl-tests, or a profiler is written as an instruction with "not run in this build";
   - its first run on real hardware should replace these notes;
   - the five CUDA programs (two CUDA probes and three CUDA-and-MPI programs) have never executed past device discovery.
2. **The "X %" target** in F5's third acceptance test is left to the learner to set before measuring, as the curriculum does.
3. **Simulated nodes in F8-10** prove correctness, not speed. The chapter says so.
4. **Open MPI was not rebuilt with CUDA support** for a GPU-aware comparison. Doing so would need network access and a GPU to be meaningful.
