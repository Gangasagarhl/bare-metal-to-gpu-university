# DS303 — Clusters and schedulers: author notes

These notes cover chapters F5-22 to F5-27 (levels L3 to L4). Course card: guide 5.10. Mapped sources: Slurm documentation, Kubernetes documentation, and the Borg paper (Verma, Pedrosa, Korupolu, Oppenheimer, Tune, Wilkes, EuroSys 2015). No curriculum milestone id was found for this course.

| Chapter | Title | Level | Figures | Labs folder |
|---|---|---|---|---|
| F5-22 | What a cluster is | L3 | 2 | `university/labs/F5-22` |
| F5-23 | Batch scheduling with Slurm | L3 | 2 | `university/labs/F5-23` |
| F5-24 | Containers | L3 | 2 | `university/labs/F5-24` |
| F5-25 | Kubernetes architecture | L3–L4 | 2 | `university/labs/F5-25` |
| F5-26 | Scheduling GPUs in clusters | L4 | 3 | `university/labs/F5-26` |
| F5-27 | Lessons from Borg | L4 | 2 | `university/labs/F5-27` |

Course labs from the card:

- **Small Slurm cluster in VMs:** F5-23 Part B. Untested in this build. Part A is a working batch-scheduler model.
- **Kubernetes local learning cluster with a replicated service:** F5-25 Part B. Untested in this build. Part A is a working control-plane model and a label-selector model.

Course forensic "Jobs pending forever" is F5-23's forensic lab (`pending.in`). Every other chapter also has a forensic lab with real evidence produced by its own listings:

- F5-22: a failed 64-node job.
- F5-24: the OOM kill.
- F5-25: a service with no endpoints.
- F5-26: GPUs idle but the job waits.
- F5-27: starvation by preemption.

The exam type P ("write job scripts") is practised in:

- the F5-23 mini-project;
- F5-26 Check yourself question 8;
- F5-26's `gpu_job.sh`.

The course project "GPU-job submission workflow for F8 labs" is the F5-26 mini-project. Its seed is `labs/F5-26/gpu_job.sh`.

## Toolchain and environment (from the logs)

- **Compiler:** g++ 13.3.0. The course flags are `-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined`.
- **MPI:** Open MPI 4.1.6. F5-22 builds with `mpic++ -DOMPI_SKIP_MPICXX`, because Open MPI's deprecated C++ bindings fail under `-Werror`.
- **CUDA:** nvcc 12.0. There is no GPU in the build container.
- **Containers:** runc 1.5.1 (OCI spec 1.3.0).
- **Kernel features:** cgroup v1 memory controller, writable. `unshare` is allowed.
- **Not installed:** Slurm, Kubernetes, container images, internet.

All six labs were run again in order at the end of the build. Each `run_lab.sh` exited with status 0.

## Listings run

| Run | Listing | Result |
|---|---|---|
| F5-22 inventory | inventory.cpp | pass (exit 0) |
| F5-22 failures | failures.cpp | pass (exit 0) |
| F5-22 pi_mpi | pi_mpi.cc via run.sh, `mpirun -np 1,2,4` | pass (exit 0). Timings vary from run to run, so the prose quotes none. |
| F5-23 batchsim | batchsim.cpp on batchsim.in | pass (exit 0) |
| F5-23 pending | batchsim.cpp on pending.in (forensic) | pass (exit 0) |
| F5-23 job_bash | job.sh under bash | pass (exit 0). **Untested under Slurm.** |
| F5-24 ns_demo | ns_demo.cpp (UTS + PID namespaces) | pass (exit 0) |
| F5-24 runc_help, runc_spec | runc, bundle.py | pass (exit 0) |
| F5-24 host_view, container_view, container_limit | inside.cc in and out of runc | pass (exit 0) |
| F5-24 oom_host | eat.cc without a limit | pass (exit 0) |
| F5-24 oom_container | eat.cc in runc with a 64 MiB limit | **expected evidence, exit 137** (SIGKILL by the OOM killer) |
| F5-24 oom_cgroup | eat.cc in a hand-made cgroup v1 group | **expected evidence, exit 137**. The counters are printed afterwards. |
| F5-25 reconcile | reconcile.cpp | pass (exit 0) |
| F5-25 endpoints | endpoints.cpp on endpoints.in (forensic) | pass (exit 0) |
| F5-25 web.yaml | manifest | **not run.** No Kubernetes in the build. |
| F5-26 gpupack | gpupack.cpp on gpupack.in | pass (exit 0) |
| F5-26 idle | gpupack.cpp on idle.in (forensic) | pass (exit 0) |
| F5-26 gpu_visible, gpu_visible_job | gpu_visible.cu (real nvcc build) | **untested on hardware.** Exit 1 with `cudaErrorNoDevice`. |
| F5-26 gpu_job_bash | gpu_job.sh under bash, with gpu_visible as the check | **untested under Slurm and on hardware.** Exit 1, passed through from the GPU check. |
| F5-27 borgsim | borgsim.cpp on borgsim.in | pass (exit 0) |
| F5-27 starve | borgsim.cpp on starve.in (forensic) | pass (exit 0) |

Check-yourself answers that need a modified program or input were checked by running modified copies in the author's scratch folder. These are F5-23 Q3 and Q4, F5-25 Q4 and Q5, F5-26 Q4 and the forensic fix, and F5-27 Q4 and Q5. The answers record what those runs printed.

## Unverified material (per chapter)

All D-sources are "title only — not opened during this build". Dossier gate G1 is open. Unverified boxes:

- **F5-22**
  - Slurm daemon and command names (`slurmctld`, `slurmd`, `sinfo`, `squeue`, `sbatch`, `srun`).
  - Kubernetes and Borg names used for comparison.
  - The MPI standard's terms.
  - The checkpoint-interval literature (the mini-project asks the learner to find and cite it).
- **F5-23**
  - The large table of Slurm commands, options, `#SBATCH` directives, partition and QOS terms, and pending-reason names (`Resources`, `Priority`, `ReqNodeNotAvail`, …).
  - The multifactor priority plugin.
  - Backfill parameters. The EASY backfill name and its attribution are in an unverified box.
  - Part B, the Slurm VM lab: the whole outline.
- **F5-24**
  - OCI runtime and image spec field names beyond what `runc spec` printed.
  - Docker, Podman and containerd names.
  - cgroup v2 file names (the build used v1).
  - Rootless containers and user namespaces.
  - The CRI.
- **F5-25**
  - All Kubernetes component, object and field names, and `kubectl` commands.
  - Node failure taints and grace periods.
  - Service proxy modes.
  - etcd's disk behaviour.
  - Part B, the learning cluster: the whole outline. `web.yaml` was never applied.
- **F5-26**
  - Slurm GRES options and configuration (`--gres`, `--gpus-per-node`, `gres.conf`, `ConstrainDevices`).
  - Kubernetes `nvidia.com/gpu` and the device plugin.
  - CUDA_VISIBLE_DEVICES renumbering. The local header only confirms the variable restricts devices.
  - MIG profiles and counts, MPS, time slicing.
  - Topology tools and policies.
  - Device file names and the cgroup v2 device mechanism.
  - Part B on a GPU cluster.
- **F5-27**
  - Every statement attributed to the Borg paper: architecture, bands, quota overselling, reclamation details, isolation, and the lessons list. A table in the chapter maps each claim to the paper section to check.
  - No number from the paper is quoted.

Tier-4 local sources that were opened and cited with line numbers:

- glibc 2.39 `bits/sched.h` (CLONE_NEW* flags) and `bits/signum-generic.h` (SIGTERM 15, SIGKILL 9), in F5-24;
- Open MPI `mpi.h` line 2909, in F5-22;
- CUDA 12.0 `cuda.h` lines 240 and 4250–4251, `cuda_runtime_api.h` lines 1409, 4936–4937 and 4946–4947, and `driver_types.h` lines 545 and 955–978, in F5-26.

## Decisions for the owner

1. **Learning-cluster tool for F5-25 Part B.** Candidates named only in an unverified box: kind, minikube, k3s. The dossier must pick one, with its documentation pages and version.
2. **Slurm VM lab (F5-23 Part B).** Decide how many VMs, which Slurm version and packaging, and whether QEMU or another hypervisor is used. Part B is untested.
3. **GPU cluster access for F5-26 Part B and the course project.** No GPU was available to this build. Someone must run `gpu_visible.cu` and `gpu_job.sh` on real Slurm and Kubernetes GPU nodes and record the output.
4. **New sources proposed for the dossier:**
   - Burns, Grant, Oppenheimer, Brewer, Wilkes, "Borg, Omega, and Kubernetes" (ACM Queue 2016);
   - Schwarzkopf et al., "Omega" (EuroSys 2013);
   - Feitelson and Rudolph on gang scheduling (JPDC 1992);
   - Jeon et al., "Analysis of Large-Scale Multi-Tenant GPU Clusters for DNN Training Workloads" (USENIX ATC 2019);
   - NVIDIA MIG and MPS guides.
   
   All are cited as title only.
5. **EASY backfill name (F5-23).** The chapter explains backfill without depending on the name. Confirm the attribution or drop it.
6. **Proposed analogy mappings (F5 "friends in different towns").** The registered mapping is "cluster scheduler = the organiser who assigns chores". Proposed additions:
   - F5-22: "node = one friend's house with its own tools"; "job = one chore that may need several friends at once".
   - F5-23: "job script = the chore card with the tools list written on top"; "backfill = letting a short chore slip into a gap that will be over before the big chore's friends are all free".
   - F5-24: "container = a sealed chore kit: the tools and instructions packed so the chore runs the same in any friend's house, done in a room with its own door, its own house number plate and a fixed share of the house's supplies".
   - F5-25: "control plane = the shared noticeboard plus the helpers who each watch one part of it and fix what differs from the wishes pinned there".
   - F5-26: "GPU islands = benches of ovens that pass trays directly"; "fragmentation = free ovens scattered so no kitchen has enough for the big cake".
   - F5-27: "priority bands = urgent chores may interrupt hobby chores, never the reverse"; "quota = each friend's yearly allowance of helper-hours".
7. **Glossary qualifiers.** Some terms already exist in other courses, so DS303 uses qualified terms to avoid duplicates:

   | DS303 term | Existing term (course) |
   |---|---|
   | Container (Linux) | Container (SP101) |
   | Namespace (Linux) | Namespace (DR301) |
   | Node (cluster) | Node (HW101) |
   | Checkpoint (job) | Checkpoint (OS401) |
   | Preemption (scheduling) | Preemption (OS201) |
   | Starvation (scheduling) | Starvation (OS201) |

   F5-22 does not define "Scheduler" again. It links to OS201's term. Merge these if the owner prefers.
8. **Root-only labs.** F5-24's runc and cgroup runs need root and a writable cgroup v1 memory controller. The chapter gives a rootless fallback path for learners. Decide whether learners' machines (cgroup v2 by default) need a v2 variant of `run.sh`.
