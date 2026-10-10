# DR402 — Porting: ARM64, RISC-V and more architectures: author notes

DR402 has chapters F4-23 to F4-30. It is level L4, 6 credits, and needs OS303 (B1–B11) first. It maps to Track D: milestones D1–D7, the optional X1/X2, and the section 10.2 arch-neutrality checklist. The labs are in `university/labs/F4-23` to `university/labs/F4-30`.

Each lab folder passes `university/labs/run_lab.sh university/labs/<ID>` with status 0. All eight were run again, one after the other, at the end of this build on 2026-10-09, after the last code change. F4-26 and F4-30 were run once more after a comment-only edit to their `run.sh`.

The table below comes from the `.log` files of those final runs. Wall-clock times, lost-update counts and the order in which CPUs come online change from run to run. The chapters say this wherever they quote those numbers.

## Toolchain (as recorded in the logs)

- **QEMU:** 8.2.2 (Debian 1:8.2.2+ds-0ubuntu1.18), TCG only (no KVM in the container).
  - `qemu-system-aarch64`: `virt` (GICv3, and GICv2 for the refusal test) and `raspi3b`.
  - `qemu-system-riscv64`: `virt` and `sifive_u`, with QEMU's bundled OpenSBI v1.3 (SBI 1.0).
  - qemu-user for ten CPUs (F4-30).
- **Cross compilers:**
  - aarch64-linux-gnu-g++ and riscv64-linux-gnu-g++ 13.3.0, with GNU ld 2.42.
  - Host g++ 13.3.0.
  - clang 18.1.3 and ld.lld 18.1.3 (F4-30).
  - Python 3 (`make_dtb.py`, `neutral_fix.py`).
- **Kernel flags** (shared builder `labs/F4-23/dlab.sh`: `rec`, `kbuild`, `reuse_record`):
  - Common: `-std=c++20 -ffreestanding -fno-exceptions -fno-rtti -O2 -Wall -Wextra -Werror`.
  - AArch64: `-mgeneral-regs-only -mstrict-align -mno-outline-atomics`.
  - RISC-V: `-march=rv64imac_zicsr_zifencei -mabi=lp64 -mcmodel=medany`, plus the empty `F4-28/include/gnu/stubs-lp64.h`.
- **Shared files:** F3-18's `kformat.h`, `kprint.*`, `serial.h`, `panic.*` and `cxxrt.cc` are compiled unchanged into every kernel. Their SHA-256 hashes are printed in each `build.out`.
- **No hardware:** nothing ran on real hardware.

## Listings run

Status meanings:

- **pass:** the expected result.
- **expected-fail:** a failure the lab is designed to produce and checks for.
- **untested on hardware:** applies to every QEMU run.

| Chapter | Run (`.log`) | Exit code | Status |
|---|---|---|---|
| F4-23 | checklist_aarch64 | 0 | pass |
| F4-23 | checklist_riscv64 | 0 | pass |
| F4-23 | checklist_x86 | 0 | pass |
| F4-23 | dump_aarch64 | 0 | pass |
| F4-23 | dump_riscv64 | 0 | pass |
| F4-23 | fdt_test | 0 | pass |
| F4-23 | forensic_int | 0 | pass (evidence run); untested on hardware |
| F4-23 | forensic_serial | 124 | expected-fail (silent kernel, stopped by the time limit); untested on hardware |
| F4-23 | tool | 0 | pass |
| F4-23 | variants | 0 | pass |
| F4-24 | boot_el1 | 0 | pass; untested on hardware |
| F4-24 | boot_el2 | 0 | pass; untested on hardware |
| F4-24 | build | 0 | pass |
| F4-24 | forensic_int | 0 | pass (evidence run); untested on hardware |
| F4-24 | forensic_serial | 124 | expected-fail (kernel stays at EL2, stopped by the time limit); untested on hardware |
| F4-24 | outline | 1 | expected-fail (link error that must happen: outline atomics) |
| F4-25 | build | 0 | pass |
| F4-25 | d2 | 0 | pass; untested on hardware |
| F4-25 | forensic_storm | 124 | expected-fail (timer storm, stopped by the time limit); untested on hardware |
| F4-25 | gicv2 | 0 | pass (clear 'unsupported' refusal expected); untested on hardware |
| F4-25 | rx_mistake | 0 | expected-fail (D2 FAILED, the common-mistake build); untested on hardware |
| F4-26 | build | 0 | pass |
| F4-26 | legacy | 0 | expected-fail (legacy virtio-mmio refused, D3 FAILED); untested on hardware |
| F4-26 | publish | 0 | pass (compiled only, three CPUs) |
| F4-26 | reorder_sim | 0 | pass (a model, not a CPU trace) |
| F4-26 | smp1 | 0 | pass; untested on hardware |
| F4-26 | smp2_el2 | 0 | pass; untested on hardware |
| F4-26 | smp4 | 0 | pass; untested on hardware |
| F4-26 | smp8 | 0 | pass; untested on hardware |
| F4-26 | smp8_slow | 0 | observation, not a pass (4000 rounds on 8 vCPUs: 278 ms in the final run; ~23 s and >30 s in earlier runs); untested on hardware |
| F4-27 | build | 0 | pass |
| F4-27 | dtb | 0 | pass |
| F4-27 | forensic_int | 0 | pass (evidence run); untested on hardware |
| F4-27 | forensic_serial | 0 | expected-fail (silent no-ranges kernel; exit 0 comes from the watchdog reset); untested on hardware |
| F4-27 | nodt | 0 | pass, 5 of 5 runs 'D4 ok'; untested on hardware |
| F4-27 | withdt | 0 | pass, 5 of 5 runs 'D4 ok'; untested on hardware |
| F4-28 | build | 0 | pass |
| F4-28 | d5 | 0 | pass; untested on hardware |
| F4-28 | forensic_int | 0 | pass (evidence run); untested on hardware |
| F4-28 | forensic_serial | 124 | expected-fail (kernel linked at 0x80000000, silent, time limit); untested on hardware |
| F4-28 | stubs | 1 | expected-fail (compile error that must happen) |
| F4-29 | all_harts | 124 | expected-fail ('only 1 of 2 harts came online'; SRST -2, time limit); untested on hardware |
| F4-29 | build | 0 | pass |
| F4-29 | forensic_nocomplete | 0 | expected-fail (1 character, D6 FAILED); untested on hardware |
| F4-29 | sifive_u | 124 | pass ('D6 ok'); exit 124 because SBI SRST returns -2 on sifive_u; untested on hardware |
| F4-29 | virt1 | 0 | pass; untested on hardware |
| F4-29 | virt4 | 0 | pass; untested on hardware |
| F4-29 | virt8 | 0 | pass; untested on hardware |
| F4-30 | first_try | 0 | expected-fail in part (4 of 10 CPUs build; the table is the evidence) |
| F4-30 | fix | 0 | pass |
| F4-30 | fixed | 0 | pass on 7 CPUs with the fixed copies; qemu-user, no kernel |
| F4-30 | forensic | 0 | pass (evidence run: SIGSEGV addresses); qemu-user |
| F4-30 | ported | 0 | pass for 5 64-bit CPUs; expected crash (exit 139) on 5 32-bit CPUs; qemu-user, no kernel |

Exit code 124 means the run was stopped by its time limit. Exit 0 on `raspi3b` comes from a watchdog reset and is **not** a verdict there; the verdict is the "D4 ok" line, and `run.sh` checks it. F4-26 `reorder_sim` is built by `run_lab.sh` with ASan and UBSan.

## Unverified boxes and untested-on-hardware statements, per chapter

Every source in every chapter is "title only — not opened during this build" (dossier gate G1 open), except the project's own Systems Curriculum and lab files.

### F4-23
- **Unverified:**
  - PC-column sources: UEFI memory map, ACPI MADT, HPET table.
  - GIC three-cell interrupt format and SGI/PPI/SPI ranges.
  - Order of the four timer interrupts.
- **Untested on hardware:** all QEMU runs.

### F4-24
- **Unverified:**
  - arm64 Image header fields and boot requirements.
  - HCR_EL2/CNTHCTL_EL2/SPSR/DAIF bits for the EL2→EL1 drop.
  - ESR exception classes.
  - PSCI function IDs.
  - Outline atomics as the GCC default, and Device-memory alignment with the MMU off.
  - Entry EL on real firmware.
- **Safety box:** none of the lab touches hardware.

### F4-25
- **Unverified:**
  - MAIR/TCR/SCTLR fields and descriptor bits.
  - The barrier and TLB sequence.
  - GICv3 register offsets and bits (GICD, GICR, ICC).
  - Firmware's programming of CNTFRQ_EL0 on real boards.
- **Untested:** QEMU TCG is more forgiving than hardware about missing barriers.

### F4-26
- **Unverified:**
  - PSCI CPU_ON/AFFINITY_INFO IDs, the −4 ALREADY_ON code, and the entry state.
  - TSO, the AArch64 model and RVWMO.
  - Barrier domains, and the claim that devices need `osh`/`sy` rather than `ish`.
  - virtio-mmio offsets and the legacy/modern versions.
  - EL3 firmware path; coherent and non-coherent DMA.
- **Course forensic:** "Works on x86, corrupts on ARM" uses `reorder_sim.cpp`. It is a **model**, labelled as such in its output, because TCG on an x86-64 host never shows reordering.

### F4-27
- **Unverified:**
  - The Pi boot chain and `config.txt` options (every one).
  - BCM2837 addresses, GPFSEL1, the 48 MHz UART clock, and the PM watchdog password and registers.
  - The PL011 registers.
  - Pi 4 (0xfe000000, GIC-400) and Pi 5 (RP1 behind PCIe, debug UART) facts.
  - A real Pi 3's CNTFRQ (19.2 MHz recalled).
- **Safety box:**
  - A separate SD card.
  - A 3.3 V USB-serial adapter wired GND/TX/RX only, with the power off.
  - Never a machine you depend on.
- **Untested-on-hardware box:** the D4 acceptance test needs a real board and was **not met** in this build.

### F4-28
- **Unverified:**
  - The OpenSBI boot convention.
  - S-mode CSR names and bits, the PTE layout and delegation.
  - The SBI extension IDs and legacy calls.
  - The board boot chain.
  - Whether hardware sets A/D or faults.

### F4-29
- **Unverified:**
  - PLIC offsets and claim/complete semantics.
  - Causes 9 and 11 in `interrupts-extended`.
  - That `set_timer` clears pending.
  - Sstc's `menvcfg` enable.
  - sifive_u's E51 monitor hart and its UART layout.
  - SBI error −2 = NOT_SUPPORTED.
- **Untested-on-hardware box:** milestone D7 (VisionFive 2) is described, not tested. `sifive_u` is the stand-in.

### F4-30
- **Unverified:**
  - The compiler helper names and ABIs.
  - Linux system-call numbers and registers for ten CPUs.
  - Every kernel-level fact for LoongArch, s390x, MIPS and PowerPC in the comparison table.
- **Untested-on-hardware box:** qemu-user only. No kernel boots in this lab.

## Decisions for the course owner

1. **F3-18 `kformat.h` treats `%ll` as `%l`** (any number of `l`s means `long`). Also, F4-23's `neutral_tests.cc` passes `0xffffffff80000000ul` to `%lx`.
   - Both are harmless on LP64. On ILP32 the test crashes, with SIGSEGV at 0xffffffff on little-endian CPUs and 0x80000000 on big-endian ones.
   - `labs/F4-30/neutral_fix.py` writes fixed copies and does not edit the originals; F4-30's "fixed" run passes on all five 32-bit CPUs.
   - **Decision:** adopt the fix in OS303's `kformat.h` (count the `l`s) and F4-23's test (`%llx`, `ull`)? Also consider `__attribute__((format(printf, …)))` on `ksnprintf`/`kprintf`.
   - The chapters currently present the unfixed state as the forensic "before".
2. **DR301's virtio code (F4-05 `virtio.cc`) is coupled to PCI.** `vio::init` takes a `PciAddr` and walks PCI capabilities.
   - D3's acceptance test asks that the C4 virtio-blk code be "shared, not copied". That needs a transport interface below the device drivers.
   - F4-26's lab uses a separate small virtio-mmio driver. F4-26 says this and makes the interface its mini-project.
   - **Decision:** refactor DR301 with a `virtio::Transport` interface?
3. **F4-23's `fdt_tool` ignores `/aliases`.** Checklist row 6 prints "no usable stdout-path" for a Pi-style devicetree (F4-27 R2), and F4-27 uses this as a teaching point and lab step. **Decision:** fix in F4-23 or keep it?
4. **F4-23's neutral-test skip message** says "(ACPI)" even when the platform uses a board table (F4-27 `nodt`). Wording fix only.
5. **Non-atomic 64-bit atomic helpers** in `labs/F4-30/port32.cc` are single-CPU only, and the chapter says so. An X1 port on SMP must lock or avoid 64-bit atomics.
6. **Ticket lock under vCPU oversubscription** (8 vCPUs on a 4-CPU host): wall time varies by two orders of magnitude with host load.
   - F4-26 `smp8_slow` ran in 278 ms in the final run, and in about 23 s and >30 s in earlier runs.
   - F4-29 uses `rounds=500` for 8 harts; 5000 rounds hung for minutes in an early run.
   - CI should keep round counts small. `smp8_slow` is an observation, never a pass condition.
7. **One unexplained early hang:** F4-27 without a devicetree, during host load. It did not reproduce in 46 later runs. The lab now boots each configuration 5 times and reports the count. The hypotheses are listed in the chapter; there is no finding.
8. **SBI SRST returns −2 on QEMU `sifive_u`** with the bundled OpenSBI v1.3, so those runs end at the time limit (124). The pass condition is the "D6 ok" line.
9. **QEMU 8.2.2's virt machine defaults to legacy virtio-mmio** (version 1). The labs pass `-global virtio-mmio.force-legacy=false`, and the driver refuses version 1 clearly (F4-26 `legacy`).
10. **D4 and D7 acceptance tests require real boards.** This build rehearses them on `raspi3b` and `sifive_u` only. Someone with a Pi 3 and a VisionFive 2 should run them and record the logs.
11. **Analogy registry (school world).** Mappings proposed in these chapters, for the registry owner to accept or change:
    - "building plan on the principal's desk = devicetree" (F4-23, F4-27).
    - "service hatch in the caretaker's office = SBI ecall" (F4-28).
    - "mailbox watched by a waiting teacher = spin-table release address" (F4-27).
    - "fire-alarm timer = watchdog reset" (F4-27).
    - "internal post where notes to different rooms may overtake each other = weak memory ordering" (F4-26).
12. **Glossary merges.** Several DR402 terms share a slug with entries from other courses, and `build.py` merges entries with identical term strings:
    - Devicetree and Phandle (DR403).
    - Hart and Sv39 (OS201).
    - Per-CPU data (OS303).
    - Interrupt storm (OS305).

    DR402's "phandle" was renamed to "Phandle" so it merges with DR403's entry instead of producing a duplicate id. The owner may want one wording per merged term.

## Glossary

`glossary.json` has 51 four-part entries, one per jargon-box term in F4-23 to F4-30. Each entry's source is the chapter source cited in its jargon box (pending verification), and its chapter list includes every chapter that links to it.

## Build check

`python3 university/build/build.py` reports no PROBLEM line for F4-23 to F4-30. Every link those chapters use resolves.

All eight chapters show "ok — n listing run(s)": F4-23 10, F4-24 6, F4-25 5, F4-26 9, F4-27 6, F4-28 5, F4-29 7, F4-30 5.
