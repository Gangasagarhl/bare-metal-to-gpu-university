# BR-10 From a laptop to a server cluster — author and lab notes

Bridge chapter (guide 6.2), level L3–L4, placed before DS303 and DG402 (prerequisite of
F5-22 and F8-07). Files: `BR-10.html`, `glossary.json` (6 new terms), this file; lab in
`university/labs/BR-10/`. Analogy world: F5 (friends in different towns), registered
mappings only (letters, phoning, everyone's watch, the organiser). No new mapping proposed.

## Listings run (all by `university/labs/run_lab.sh university/labs/BR-10`, exit status 0)

Final run: 2026-10-10 about 03:07Z, g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0,
mpirun (Open MPI) 4.1.6, Linux x86_64 container with 4 virtual CPUs (KVM guest), 1 socket,
1 NUMA node. No Slurm, no second machine, no BMC.

| Run | Listing / step | Result |
|---|---|---|
| R1 | `laptop.cpp` (ASan+UBSan) | pass, exit 0 |
| R2 | `cluster.cpp` (ASan+UBSan) | pass, exit 0 (speedup 0.91 in this run: host noise) |
| R3 | `compare_small` (run.sh, -O2) | pass, exit 0; measured on a shared VM |
| R4 | `compare_large` (run.sh, -O2) | pass, exit 0; measured on a shared VM |
| R5 | `noisy.cpp` (ASan+UBSan, 120 s limit) | pass, exit 0; measured on a shared VM |
| R6 | `hang.cpp` | **expected hang**: exit 124 from the 5 s limit (`hang.timeout`) |
| R7 | `timeouts.cpp` | pass, exit 0 |
| R8 | `clocks.cpp` | pass, exit 0 (clock offsets injected) |
| R9 | `job_mpi` (run.sh, mpirun -np 3) | pass, exit 0 |
| R10 | `job_mpi_stall` (run.sh) | **expected hang**: exit 124 from `timeout 8` |
| R11 | `slurm_script` (bash cluster_job.sh) | pass, exit 0 — **untested under Slurm** |
| R12 | `numa` (lscpu, sysfs) | pass, exit 0 |
| R13 | `bottleneck.cpp` | pass, exit 0 (exercise values) |
| R14 | `forensic.cpp` (evidence generator) | pass, exit 0 |
| R15 | `bench_copy` (cmp with F2-51/bench.hpp) | pass, identical |

No `.expect-fail` listings. `job_mpi.cc` is `.cc` so run_lab.sh does not build it with g++
(it needs mpic++; same convention as F5-22). `bench.hpp` is a byte copy of
`labs/F2-51/bench.hpp` (checked by R15 at every run): if F2-51's harness changes, R15 fails
and the copy must be refreshed.

**Numbers quoted in the prose come from the recorded run above** (R3 19.87/11.55 ms, 1.72;
R4 497.39/287.77 ms, 1.73; R2 0.91 and 140 %; R5 Part A 110.46/116.68/231.76 ms, 2.84 and
1.91; R5 Part B 52.71/47.73/160.01/69.99 ms, n2 133.8/163.8 ms, n1/n3 39.9/52.6 ms, 2 of 12
tasks; R7 times 3.0, 6.6, 8.8, 28.7, 504.4, 505.9, 1205.1 ms and 503 ms; R8 offsets and
delays, 7 of 12; R14 values in the forensic key; R4 busy 236.4/280.0/263.9). The timing
runs change on every rerun (shared VM). Whoever reruns the lab must update Layer 3, the
Worked example, Figure 2, Figure 3, the Lab's expected observations, the Answers and the
forensic key, or keep the recorded `.out` files.

Arithmetic in the worked examples was checked with Python during authoring (6.623, 0.5734,
6.997, 5.2, 13.0, 8.0, −299.616 + 300.001 = 0.385).

## Unverified boxes (AH-18)

1. Layer 2, Change 2: no Redfish resource paths/properties/actions and no IPMI command
   numbers are given; check power control in the DMTF Redfish Specification/schemas and the
   IPMI v2.0 Specification before fencing a real node.
2. Layer 3, "From this model to a real scheduler": all Slurm names in Listing 10
   (`#SBATCH`, `--job-name`, `--nodes`, `--ntasks`, `--time`, `--output`, `%j`, `srun`,
   `SLURM_JOB_ID`, `SLURM_JOB_NODELIST`) and the whole-node option are from memory, as in
   F5-23's unverified table. Only bash ran the script.

Plus one "untested on hardware" warning box (Change 5: NUMA on a multi-socket server).

## Claims that could not be verified in this build

- All D-sources (Slurm docs, Kleppmann ch. 8, van Steen and Tanenbaum, Lamport 1978, Redfish
  and IPMI, SRE book, MPI Standard, ACPI SRAT/SLIT, Linux man-pages, ISO C++ chrono) are
  "title only — not opened"; dossier gate G1 open. The container has no man pages installed.
- Semantics of `PR_SET_PDEATHSIG`, `MSG_NOSIGNAL`, `sched_setaffinity`, `poll` with -1 are
  from memory (D9); the runs behaved as described (no leftover processes after R6, no
  SIGPIPE when n3 died in R7, pinning used in R5).
- The general statements about real clusters (time-synchronisation services, BMC on standby
  power, correlated failures through shared power or switches) are conceptual and tagged to
  D2/D3/D5; no hardware numbers are given.
- Local evidence opened: `timeout --help` (exit status 124), `lscpu`, sysfs (L1).

## Decisions for the owner

1. **The card's lab ("a three-node virtual cluster under Slurm") is replaced** in this
   build by the university's own three-process virtual cluster (`cluster.hpp`) plus a Slurm
   batch script run by bash, because the container has no Slurm and no VMs. The real Slurm
   version is lab step 8, untested; it reuses F5-23's VM plan (also unverified). Approve, or
   schedule a Lab Engineer run on a host with Slurm.
2. **Clock skew is injected**, not observed (one host has one clock). Approve the honest
   labelling, or provide two machines for a real measurement.
3. **Fencing is a SIGKILL stand-in** for a BMC power-off; labelled as such in the log line.
4. Timing results come from a visibly noisy shared VM (R2's speedup 0.91 versus R3's 1.72,
   one minute apart). The chapter uses that noise as teaching material (Trap 1). If the
   owner prefers stable numbers for publication, rerun on a quiet dedicated machine and
   update the prose numbers listed above.
5. Glossary: "Straggler" lists F5-39 under chapters because that chapter uses the word
   without a glossary entry; the Integrator may want F5-39 to link it.
6. The chapter is long for a bridge (about 12,500 words including tables, answers and
   sources); the Editor may move the line-by-line table of `cluster.hpp` into the lab page.
