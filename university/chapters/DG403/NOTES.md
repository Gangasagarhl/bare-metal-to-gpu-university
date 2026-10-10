# DG403 — Parallelism strategies and fault tolerance: author notes

Six chapters, F8-12 to F8-17. Levels: F8-12 to F8-16 are L4, F8-17 is L5. Course card (guide 5.13): labs F7 and F8, forensic "The hung job", exam P (design review of a parallelism plan), project "a fault-tolerant training run that resumes from checkpoint".

- Labs are in `university/labs/F8-12` to `university/labs/F8-17`. Each passes `university/labs/run_lab.sh university/labs/<ID>` with exit status 0 (all six re-run after the chapters were finished).
- Every fragment passes the author's checks: `html.parser` balance, all ids prefixed with the chapter id, no URLs, no `<script>`, the 21 sections plus Answers in order, a jargon box, a Transition box, every claim tag pointing at a source item.
- `glossary.json` holds 30 entries, one per jargon-box term. Three of them have exactly the same term as an existing entry and will be merged by `build.py` (the other course's text is kept; F8-1x is added to its chapter list):
  - "Reduce-scatter" and "Scaling efficiency" (DG401);
  - "Node-local rank" (DG402).
  - "Communication-computation overlap" is spelled with a hyphen, as in DG402, so that the two entries merge instead of producing two elements with the same id.
- Linked but not redefined: Collective, NCCL / RCCL, Watchdog (curriculum seed); Alpha–beta model (HW301); Rank (MPI), Checkpoint (job), Gang scheduling, Preemption (scheduling), Device visibility (GPU), Backfill scheduling (DS303); Failure detector, Heartbeat (DS301); MTBF and MTTR (DS304).

## What this build could and could not do

- **Available:** g++ 13.3.0, Open MPI 4.1.6 (MPI 3.1), nvcc 12.0, bash; 4 logical CPUs.
- **Not available:** a GPU, NCCL or RCCL (no headers), a cluster scheduler (no Slurm), a second machine.
- **Substitutes:** MPI processes on one computer play the GPUs; sleeps play GPU kernels (F8-14, F8-16); `SIGTERM` from `run.sh` plays the scheduler's warning (F8-17); folders named `node-a-local-tmp` and `node-b-local-tmp` play two nodes' local disks (F8-17 forensic).
- **Primary sources opened:** only installed headers (tier 4): Open MPI `mpi.h` (line numbers in each chapter's L1), CUDA 12.0 `cuda_runtime_api.h` and `driver_types.h`, glibc `stdio.h`, `unistd.h` and `bits/signum-*.h`.
- **Everything else** is cited "Title only — not opened during this build; the Source Researcher must confirm the edition and section (dossier gate G1 open)".
- **No hardware number of any real product appears.** Model inputs are labelled invented (TN-2 in F8-12; the byte counts and 64 GiB budget in F8-15; MTBF, C and R in F8-17).

## Listings and their status

| Chapter | Pass | Expected fail / evidence | Untested on hardware |
|---|---|---|---|
| F8-12 | `dp_train.cc` (np 1, 2, 4: MATCH, max 5.96e-08, replica spread 0), `dp_cost.cpp` (sweep) | `slow_step` (forensic: bucket traces k = 16 and k = 1; the evidence is the slow trace, exit 0) | GPU extension of the lab |
| F8-13 | `tp_mlp.cc` (np 1, 2, 4: MATCH; `ffn=30` np 2 MATCH), `tp_cost.cpp` | `tp_refuse` (exit 2, refused split); `tp_ffn30_np4` (forensic: 7 columns per rank, DIFFERENT, exit 0) | GPU extension of the lab |
| F8-14 | `pipe_sim.cpp` (bubble = formula for all 7 cases), `pipe_mpi.cc` M = 1, 4, 8, 16 (within 20 %, gradients exactly equal) | `pipe_slow` (forensic: stage 2 × 3, "NOT within 20 %", exit 0) | GPU extension of the lab |
| F8-15 | `zero_mem.cpp`, `zero_os.cc` (np 1, 2, 4: MATCH; `ckpt=25`: MATCH) | `zero_bug` (forensic: rank 0's checkpoint loaded everywhere, DIFFERENT, exit 0) | GPU extension of the lab |
| F8-16 | `ft_train.cc` + `restart.sh`: `ft_clean`, `ft_hang` (watchdog, exit 42, restart), `ft_kill` (exit 137, restart), `ft_corrupt` (fallback to `ckpt.prev`); all end with the clean run's checksum e31e3fa0a92a7485 | `hung` (course forensic "The hung job": watchdog off, killed by a 6 s limit, exit 137 expected) | GPU extension (NCCL communicator abort) |
| F8-17 | `ckpt_interval.cpp`, `worker.cpp`, `train_job.sh` + worker under bash (`job_preempt`), `local_rank.cc` (np 8) | `restart_zero` (forensic: requeue onto another "node" restarts from step 0, exit 0) | `pick_gpu.cu` (built with nvcc 12.0; run prints `cudaErrorNoDevice`, exit 1); `train_job.sh` under a real scheduler |

**Timing-dependent outputs:**

- F8-14 `pipe_M*` and `pipe_slow`: milliseconds and the measured bubble vary from run to run on the shared container. The chapter quotes only approximate values ("roughly 1,440 ms", "about 0.55") and the formula; the "within 20 %" verdicts held in every recorded run.
- F8-16 output timestamps vary by a few milliseconds. The chapter says "about". Open MPI's `ORTE_ERROR_LOG` lines appear in some runs and not in others, when several ranks call `MPI_Abort` at the same moment. The chapter says so.
- F8-17 `job_preempt` and `restart_zero`: the step at which `SIGTERM` arrives (16 in the recorded run) depends on timing. The chapter says it can differ by one.

**Trial runs not kept as lab records** (the text says so where it mentions them):

- F8-12: a trial in which ranks issued their bucket all-reduces in different orders ended with an Open MPI error about a truncated message.
- F8-13: `ffn=34`.
- F8-16: an early version of `restart.sh` used a plain `timeout` (SIGTERM). Open MPI forwarded the signal and woke the stopped rank, which is why `timeout -s KILL` is used.

## Unverified boxes, per chapter

- **F8-12:** PyTorch DDP bucket defaults and behaviour (default bucket size as a constructor argument, reverse parameter order, rebuilding after the first iteration), and the NCCL/RCCL environment variables that limit the multiprocessors their kernels occupy. Large-batch learning-rate scaling is cited from Goyal et al., title only.
- **F8-13:** sequence parallelism (Korthikanti et al.) and Megatron's use of reduce-scatter and all-gather around token-wise operations.
- **F8-14:** the GPipe paper's rule of thumb (M ≥ about 4K, bubble negligible). Only the formula, which the chapter derives, is used.
- **F8-15:**
  - the ZeRO paper's K = 12 and 16Ψ notation and its 7.5-billion-parameter, 64-GPU example;
  - PyTorch FSDP sharding strategies and DeepSpeed ZeRO stage configuration names.
- **F8-16:**
  - ULFM status in the MPI standard and in Open MPI;
  - NCCL `ncclCommAbort`, `ncclCommGetAsyncError` and non-blocking communicators;
  - PyTorch's `init_process_group` timeout and the `NCCL_ASYNC_ERROR_HANDLING` / `TORCH_NCCL_ASYNC_ERROR_HANDLING` variables.
- **F8-17:**
  - every Slurm option and variable in `train_job.sh` (`--gpus-per-node`, `--signal=B:TERM@120`, `--requeue`, `--open-mode=append`, `%j`, `SLURM_JOB_ID`, `SLURM_RESTART_COUNT`, `scontrol requeue`, time-limit TERM then KILL);
  - `CUDA_VISIBLE_DEVICES` renumbering and the per-task GPU binding alternatives.

Other claims that rest on memory but are attributed to a named source with a claim tag (to be checked by the Source Researcher):

- POSIX atomicity of `rename` and the directory-fsync caveat (F8-16 D3).
- MPI-3.1 not defining continuation after a process failure (F8-16 D2).
- ZeRO stage 3 communication of about 1.5 times data parallelism (F8-15 D1).
- The GPipe overhead form O((K − 1)/(M + K − 1)) (F8-14 D1).
- The 1F1B schedule appearing in PipeDream and Megatron-LM 2021 (F8-14 D2, D4).

## Sources outside the guide's registry (proposed additions)

| Source | Used in | Why |
|---|---|---|
| Goyal et al., "Accurate, Large Minibatch SGD: Training ImageNet in 1 Hour" | F8-12 | global batch and learning-rate scaling |
| Korthikanti et al., "Reducing Activation Recomputation in Large Transformer Models" | F8-13 (unverified box) | sequence parallelism |
| Narayanan et al., "PipeDream: Generalized Pipeline Parallelism for DNN Training" (SOSP 2019) | F8-14 | 1F1B schedule |
| Narayanan et al., "Efficient Large-Scale Language Model Training on GPU Clusters Using Megatron-LM" (SC 2021) | F8-14 | combined parallelism, interleaved schedule |
| Kingma and Ba, "Adam: A Method for Stochastic Optimization" | F8-15 | the optimizer whose state is sharded |
| Micikevicius et al., "Mixed Precision Training" | F8-15 | FP32 master weights (16 bytes per parameter accounting) |
| The Open Group Base Specifications (POSIX.1): `rename`, `fsync` | F8-16 | atomic checkpoint replacement |
| Young, "A first order approximation to the optimum checkpoint interval" (CACM 1974) | F8-17 | T* = √(2CM) |
| Daly, "A higher order estimate of the optimum checkpoint interval for restart dumps" (FGCS 2006) | F8-17 | refinement when C/M is not small |

Registry sources used: curriculum item 85 (Megatron-LM, GPipe, ZeRO, Horovod), the PyTorch DDP paper, the MPI Standard (item 77), NCCL/RCCL documentation (guide 4.6), the Slurm documentation (guide 4.5), Patarasuk and Yuan, IEEE 754, Kirk and Hwu.

## Proposed analogy mappings (F8 world: the great kitchen hall)

Registered mappings used: "a chain of kitchen halls with a delivery service"; ring all-reduce = "passing bowls around a round table". Proposals, for the analogy registry:

- F8-12: replica = "a hall with its own full copy of the recipe book".
- F8-13: tensor parallelism = "every hall prepares a slice of every plate, and the slices are combined before the plate leaves".
- F8-14: pipeline stage = "a hall that does one course and hands the tray to the next hall"; bubble = "halls standing idle while the first trays travel down the line and the last comments travel back".
- F8-15: sharded optimizer state = "each hall keeps only its own pages of the shared recipe notebook and reads the others' pages aloud when they are needed".
- F8-16: hung rank = "a cook at the round table who stopped passing bowls without saying anything"; watchdog = "the steward's kitchen timer"; checkpoint = "the evening's recipe card written in ink and filed in the safe".
- F8-17: cluster scheduler = "the booking office that gives a banquet a set of halls for a fixed evening"; preemption warning = "the office's knock on the door two minutes before the hall must be handed over"; node-local storage = "a cupboard that stays in its hall".

People named in stories and scenarios (invented): Mateo, Ayasha, Tomás (F8-14); Imani, Yusuf (F8-15); Ingrid, Sakura (F8-16); Wiremu, Olu (F8-17). F8-12 and F8-13 names are in their files.

## Milestones, forensic, exam and project

- **Milestone F7 (optional), quoted verbatim:**
  - Part 1, tensor parallelism: F8-13's lab.
  - Part 2, pipeline: F8-14's lab. Acceptance "bubble within 20 % of the GPipe formula" is printed by `pipe_mpi.cc`.
- **Milestone F8:** F8-16's lab, with goal and acceptance quoted verbatim. R2 and R3 meet the acceptance for the toy job, and F8-17 adds the scheduled restart.
- **Course forensic "The hung job":** F8-16 forensic lab (`hung.out`).
- Each other chapter has its own forensic:
  - F8-12: slow step from small buckets;
  - F8-13: an uneven split that silently changes the model;
  - F8-14: an unbalanced pipeline stage;
  - F8-15: a sharded checkpoint saved from rank 0 only;
  - F8-17: a requeue that restarts from step 0.
- **Exam P (design review of a parallelism plan):** practised in the F8-14 mini-project (the full plan, 48 layers on 32 GPUs) and the F8-15 mini-project (the memory section), and summarised in F8-17's project section. A sample exam question bank is not written; that is an owner decision (below).
- **Course project (fault-tolerant run resuming from checkpoint):**
  - F8-16 mini-project: watchdog, atomic checkpoints, restart, three injected failures; rubric of 20 points.
  - F8-17 mini-project: the scheduled part, with requeue, the computed interval and GPU mapping; rubric of 20 points.

## Decisions for the owner

1. **F8-17 level.** The course card spans L4–L5. F8-17 is marked L5 and is longer than the 3,000-word guideline (about 5,000 words including listing tables), because it carries the scheduled half of the course project. It could be shortened by moving the Monte Carlo listing to a lab-only appendix.
2. **Overlap with DS303 F5-26.** F8-17 links to F5-26 for GPU resources, GRES and device visibility, and does not repeat them. It focuses on what a distributed training job adds: gang allocation, warning signals, requeue, node-local rank and the checkpoint interval. Please confirm that the split between the two courses is acceptable.
3. **Real-scheduler testing.** `train_job.sh` ran only under bash (dry run). Its `#SBATCH` lines are unverified. A run on a real Slurm cluster is needed before the listing is promoted out of its unverified box.
4. **Outside-registry sources** (table above) need the Source Researcher to add them or to propose registry alternatives.
5. **Glossary merges.** Three exact-term merges are listed above. If the owner prefers DG403-specific wording (for example, Reduce-scatter's analogy in the kitchen world), the DG401 entry needs to be edited, because `build.py` keeps the first entry.
6. **Exam P materials.** The design-review rubric exists only inside the mini-projects. A separate exam brief (a model, a cluster, and the questions the review must answer) could be written for the course page.
