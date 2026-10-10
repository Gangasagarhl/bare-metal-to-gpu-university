# MP5 — A multi-node all-reduce: author notes

One handbook, `MP5.html` (front matter `id: MP5`, `course: MP`, level L5). Prerequisite on the card: DG402. The handbook also links DG401, DG403 (milestone 4) and SE501 (gates).

- Starter lab: `university/labs/MP5/`. It passes `university/labs/run_lab.sh university/labs/MP5` with exit status 0. A full run takes about 1.5 minutes on the shared 4-CPU container.
- `python3 university/build/build.py` reports no PROBLEM line for MP5.
- Fragment checks:
  - `html.parser` balance;
  - all 62 ids prefixed `MP5`, no duplicates;
  - no URLs, no `<script>`, no event handlers;
  - every `href="#MP5…"` resolves;
  - every `data-src`/`data-run` file exists;
  - the 20 template `<h2>` sections plus Answers, in order (the jargon box and Transition box have no `<h2>`, as the format requires).
- Length: about 9,400 words outside code and SVG, including all tables (rubric, gates, code tables, troubleshooting). The brief's guide for L5 is about 3,000 words plus project material; most of the overrun is project material. **Decision for the owner:** trim or keep.

## How the template was adapted to a project

All 21 sections are present, with exact headings and ids. Project material sits in `<h3>` subsections:

| Project material | Where it is |
|---|---|
| Milestones M1–M4: entry criteria, tasks, curriculum acceptance tests quoted verbatim, evidence, chapters built on | Layer 3 (`#MP5-m1` … `#MP5-m4`), plus the cost models (`#MP5-model`) |
| Hardware, unverified boxes, the pre-set targets table to sign at R1 | How the hardware actually does it (`#MP5-targets`) |
| Risks | Common mistakes (an 8-row risk table), safety box (`#MP5-safety`) |
| Incident / forensic write-up template, and a forensic case built from a real broken run | Forensic lab |
| Deliverables, gates R0–R4 (checklist, when, who signs), rubric with four level descriptors per criterion | Mini-project (`#MP5-deliverables`, `#MP5-gates`, `#MP5-rubric`) |

- **Figures:** Figure 1 is the architecture (layers, Link interface, transports, harness, watchdog). Figure 2 is the milestone and gate timeline.
- **Rubric weights:** exactly as on the card: 30 / 25 / 25 / 20.
- **Level descriptors** (Excellent / Good / Adequate / Insufficient, with percentage bands of the weight) are this handbook's proposal, and the page says so.

## Listings and their status

All runs are on one shared container: 4 logical CPUs, load average around 30 during the runs because sibling builds were running, no GPU, no RDMA device.

| Step | Listing | Result |
|---|---|---|
| `mp5_suite` | `mp5_check.h` + `mp5_suite.cpp` (g++ 13.3, ASan/UBSan) | pass, exit 0: 141 cases × 2 types × N = 1, 2, 3, 4, 5, 8, all PASS |
| `mp5_mutants` | `mp5_mutants.cpp` (ASan/UBSan) | pass, exit 0: the real ring fails 0 of 144; all four mutants killed; `split_floor` survives the naive suite (0 of 6) |
| `mp5_faults` | `mp5_faults.cpp` (ASan/UBSan) | pass, exit 0: 3 of 3 survivors return an error within 3 s, aborted buffers wrong, recovery correct |
| `mp5_forensic` | `mp5_forensic.cpp` (ASan/UBSan) | exit 0; this is evidence by design: build B (`Mutant::split_floor`) gives wrong elements for count mod 4 ≠ 0 |
| `mpi_ring` | `mp5_ring_mpi.cc` (Open MPI 4.1.6, -O2), np 2, 3, 4 | pass, exit 0: 56 of 56 on each process count |
| `bench` | `mp5_bench.cc` (-O2, threads) | pass, exit 0; **timings, noisy** |
| `fit` | `mp5_fit.py` (Python 3, numpy 2.5.3) | pass, exit 0; **depends on `bench.out`** |
| `mp5_kernel` | `mp5_kernel.cu` (nvcc 12.0) | untested on hardware: exit 1, `cudaErrorNoDevice` |
| `ptx` | `nvcc -ptx` of `mp5_kernel.cu` | exit 0; compiler output only, untested on hardware |

- No expected-fail compile listings.
- The `.cc` extension is used for MPI and -O2 sources, so `run_lab.sh` does not build them with sanitizers; this follows the DG402 convention.

**Do not rerun without updating the quoted numbers.** The handbook quotes these values from the recorded `bench.out` / `fit.out`:

- the N = 4 fit: α 33.10 µs, β 2.875 GB/s;
- 64 MiB: model 85,866 µs, measured 99,130 µs (ratio 0.87);
- median errors 0.06 and 0.04.

They appear in worked example (b), the Lab expectations and Answer 8. All other quoted outputs are deterministic and safe to rerun:

- suite counts and float ratios: 1.000 for N = 2, 0.527 for N = 8;
- mutant counts: 96 of 144, 4 of 6, and the other mutant counts;
- MPI bit-difference counts: 0; 1,461,849 of 6,731,058; 3,148,852 of 8,974,744;
- forensic values.

**Trial runs not kept as records:** two earlier runs of the same bench and fit, during development, while sibling jobs loaded the machine. They gave a fitted α for N = 2 of about 4.5 µs and 43 µs. Worked example (b) and Answer 8 mention them as "about ten times apart", and the text says they are not lab records.

## Unverified boxes and claims

1. **Hardware section, first box** (untested on hardware). Not stated, and to be checked in the documents named:
   - NCCL/RCCL ring and tree selection and channels;
   - the options of all_reduce_perf and perftest;
   - the switches that turn GPUDirect RDMA on and off, and the kernel or driver components it needs;
   - the AMD equivalents.
2. **Hardware section, second box:** no GPU, NIC or link number appears. Line rates go into the learner's dossier from the vendor documents. The CPU α and β must never be used as targets.
3. **Gates, decision box:** whether R0 and R3 are held, and who signs when no mentor is available (see the decisions below).

**Title-only sources** (dossier gate G1 open):

- D1 Patarasuk and Yuan;
- D2 Thakur, Rabenseifner, Gropp;
- D3 nccl-tests / rccl-tests performance notes;
- D4 NCCL and RCCL documentation;
- D5 InfiniBand specification, rdma-core, GPUDirect RDMA, ROCm GPU-aware networking;
- D6 Higham, "Accuracy and Stability of Numerical Algorithms". This one is used for the summation bound (N − 1)·u·Σ|x|, as CU302 also does.

**Opened in this build:**

- the installed headers: H1 Open MPI 4.1.6 `mpi.h`, H2 CUDA 12.0 `cuda_runtime_api.h`, both checked by the compilers;
- the guide (11.6, 11.2, "Honesty first");
- the curriculum (13.4, 14.2, 14.3).

The curriculum and the guide are cited as maps, not sources (C1, C2).

**Conceptual claim with a title-only tag:** "RDMA lets a remote peer access registered memory, so keep experiment networks isolated" (safety box, D5).

## Untested on hardware

- Everything in milestones 1–4 on GPUs, two nodes and RDMA:
  - perftest, nccl-tests, rccl-tests;
  - GPUDirect RDMA on and off;
  - profiler timelines;
  - fault injection on real nodes.
- `mp5_kernel.cu` never executed past device discovery. Its peer-access printout and the `reduceInto` check need a multi-GPU run before release.
- The curriculum acceptance tests F2, F5, F6 and F8 are quoted verbatim but **none was evaluated** in this build. The starter lab shows the CPU stand-in of F2's correctness test, scaled down: up to 16 MiB with sanitizers, not 1 GiB. F8-05 already ran 1 GiB on threads.

## Glossary

`glossary.json` holds 5 new entries:

- Link (transport interface);
- Traffic invariant (ring all-reduce);
- Pre-set target (performance);
- Model-versus-measurement report;
- Aborted collective (buffer state).

**Linked, not redefined:**

- All-reduce, Ring all-reduce, Reduce-scatter, All-gather, Segment and chunk, Bandwidth-optimal, algbw, busbw (DG401);
- Hierarchical all-reduce, Node-local rank, Summation order, GPU-aware MPI, Host staging, Communication-computation overlap (DG402);
- Watchdog, Abort, Restart from checkpoint (DG403);
- Alpha–beta model (HW301);
- GPUDirect RDMA (HW205);
- perftest (DS401);
- Unit roundoff, Tolerance (test) (CU302);
- Mutation testing (DR401);
- Least-squares fit and residual (DN201);
- Warm-up (HP401);
- Fault injection (DS402);
- Mega project, Design-review gate, Entry criteria, Review packet, Gate outcome, Claims ledger, Stated limitation (SE501);
- Blameless post-mortem (SE402).

## Analogy

Only registered mappings are used: the chain of kitchen halls with a delivery service, and passing bowls around a round table. Three DG-course proposals are also used: building = node, road = network, station = local rank. The new image of a "hatch between two halls" for the Link interface (jargon box) is a **proposal** for the analogy registry.

People named in stories (invented): Lindiwe (hook), Tomasz and Amara (forensic).

## Decisions for the owner

1. **R0 and R3.** The card lists only R1, R2 and R4. The handbook describes R0 and R3 as "not on the card; owner decides", and recommends R3 before fault injection on shared nodes. The owner also decides who signs when no human mentor is available.
2. **Rubric level descriptors.** The four-band descriptors are a proposal; please approve or edit them.
3. **Targets.** X₁, X₂, the model tolerance, the overlap target and the abort time are blank in the R1 table on purpose (curriculum 13.4). No default is suggested.
4. **Length.** About 9,400 words including tables.
5. **Forensic evidence pack.** The pack is a real run of a deliberately broken build (`Mutant::split_floor`). The guide's `_evidence/` folder and README convention was not created, because no such folder exists elsewhere in the repository yet. Its "how produced" part is inline in the answer key, as the other courses do.
6. **Next link.** MP8 is linked by its card id, because no MP8 handbook exists yet. MP3 and MP4 are linked as related projects.
