# MP1 Own OS on a real board and on a server — author's notes

Handbook `MP1.html` (front matter `id: MP1`, `course: MP`, level L5, prereqs OS304, DR302,
DR402, DR403, DS304 recommended), starter lab in `university/labs/MP1`, glossary
`glossary.json` (5 new four-part entries: Regression matrix, Regression baseline,
Host-side timestamping, Platform difference record, Remote shell). Every other term is
linked to its existing entry (boot timeline, device record, generic kernel image, network
boot, PXE, PSCI, isa-debug-exit, flaky test, quarantine, design-review gate, entry criteria,
final defence, kernel (operating system)).

## Toolchain of this build

- QEMU 8.2.2 (Debian 1:8.2.2+ds-0ubuntu1.18), TCG only, no KVM, in a cloud container.
- g++ and aarch64-linux-gnu-g++ 13.3.0, GNU ld 2.42, Python 3.13.
- Host test built by `run_lab.sh` with `-std=c++20 -Wall -Wextra -Wpedantic -Werror -g
  -fsanitize=address,undefined`.
- No real hardware, no Linux image in the container.

## Listings and runs

`university/labs/run_lab.sh university/labs/MP1` ends with exit code 0 (last full run
2026-10-10).

| Run | What | Result |
|---|---|---|
| R1 bootargs_test | host test of the command-line parser, ASan/UBSan | pass (10 of 10) |
| R2 build | build_all.sh: 2 MP1 kernels + reused D1, D3, OS303 kernels; hashes | pass |
| R3 matrix | 11 rows (10 counted, 1 quarantined) in QEMU | pass (10 of 10) |
| R4/R5 serial_x86/a64 | stamped serial logs of the smp4 rows | pass |
| R6 flaky | quarantined OS303 B9 row, 5 idle + 3 loaded attempts | observation; last run: 5/5 idle passed, 2/3 loaded passed (one FAIL, ratio 2.11 > 2.00) |
| R7 regress_matrix | MP1 rows built with -DMP1_MAX_CPUS=2 | expected fail (exit 1, 2 rows FAIL) |
| R8 regress_compare | baseline against R7 | expected fail (exit 1, 2 regressions) |
| R9 forensic_matrix | AArch64 rows of matrix_after.txt | pass (that is the point of the lab) |
| R10/R11 forensic_before/after | stamped logs | pass |
| R12 forensic_timeline | stage comparison | pass (root-device grew by about 3000 ms) |
| R13 budget | 1000 ms boot budget on the after log | expected fail (exit 1, EXCEEDED) |

All runs are QEMU/TCG: **untested on hardware**. Milestones 2–5 (board boot, server UEFI
network boot with NVMe and NIC, remote shell, Linux boot comparison) have no runs at all in
this build; the handbook describes them from the guide card and the curriculum and marks
them untested.

## Unverified boxes in the handbook

1. Linux boot-time markers and what Linux's own timestamps measure (Layer 3, timeline) —
   D14 not opened.
2. UEFI network boot methods, the protocols a loader uses to fetch further files, and DHCP
   options (Layer 3, network boot) — D8, D9 not opened.
3. When a platform must use x2APIC and how the MADT describes such processors (hardware
   section) — D3, D4 not opened; MADT layouts inherit F3-24's unverified box.

All named documents other than the guide (D1) and the curriculum (D2) are "title only —
not opened during this build; dossier gate G1 open".

## Proposed analogy mappings (F3/F4 school world)

Please review for the registry:
- regression matrix = the drill timetable
- regression baseline = last term's drill record, kept on file
- host-side timestamping = one stopwatch held by the inspector
- network boot = the rule book arriving by courier, arranged in advance
- platform difference record = one page added to the rule book
- remote shell = the office window, reached by telephone from another building

## Decisions for the owner

1. **One kernel versus the three teaching kernels.** The earlier courses did not build one
   kernel: OS302 (higher-half x86-64), OS303 (a separate 32-bit teaching kernel for B9–B13),
   OS304 and DR402 (AArch64) are separate code bases. Milestone 1 therefore starts with a
   merge, which the handbook makes the first task and the main R1 risk. The starter lab
   reuses each kernel unchanged as matrix rows and adds a small new arch-neutral MP1
   kernel for both architectures; it does not do the merge.
2. **Flaky B9 fairness row.** The OS303 B9 test (max/min ratio <= 2.00) fails under TCG
   with host load, and also failed with `-smp 4 -cpu max`. It runs quarantined
   (`~os303-threads`, smp 1, qemu64). The owner should decide whether OS303's bound or
   its configuration changes; this handbook does not edit OS303.
3. **Boot path of the starter.** The x86-64 MP1 kernel boots by Multiboot through QEMU
   `-kernel` (F3-18's path), not by the UEFI loader the project needs at milestone 3.
4. **Milestone 4 acceptance.** The card says "remote shell or file service"; the handbook
   proposes concrete acceptance tests (built from C9 and B18) for the owner to approve.
5. **"One kernel image".** Interpreted as one source tree and one image per architecture,
   with every other difference in the platform difference record; the rubric descriptors
   are this handbook's proposal.
6. **Signers of R0–R4.** The guide does not name signers; the handbook proposes peers and
   a mentor where available, with the hardware owner signing every safety item.
7. **Style.** `run.sh` has some lines over 100 characters (long commands kept on one line
   for the log records); C++, Python and build_all.sh were wrapped.
8. **Length.** The chapter is about 14,000 words including tables, gates, rubric and
   project material; the prose part is inside the L5 range, the rest is project material.
9. **Flaky observations vary.** R6 is recorded as an observation, never a pass condition;
   the handbook does not quote its counts in prose, because they change from run to run.
