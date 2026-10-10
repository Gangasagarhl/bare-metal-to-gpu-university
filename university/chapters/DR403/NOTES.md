# DR403 — SoCs and boards: author notes

Chapters F4-31 to F4-37, level L4, 4 credits. Maps to curriculum section 11 (platforms and SoCs), milestones D8 to D10. Project: D10. Basis for RB403 and DN302.
Labs are in `university/labs/F4-31` to `university/labs/F4-37`.

| Chapter | Title | Unverified boxes |
|---|---|---|
| F4-31 | How SoCs differ from PCs | 5 |
| F4-32 | The common Arm boot chain | 7 |
| F4-33 | SystemReady and EBBR | 8 |
| F4-34 | Platform families and their documentation | 3 |
| F4-35 | A devicetree-driven driver model | 5 |
| F4-36 | SD and eMMC storage | 6 |
| F4-37 | One kernel image, two platforms | 5 |

Each lab folder passes `university/labs/run_lab.sh university/labs/<ID>` with status 0. All seven were run again, one after the other, at the end of this build on 2026-10-09, after the last change. The table under "Listings run" is generated from the `.log` files of that final sweep.

## Toolchain (as recorded in the logs)

- **QEMU:** 8.2.2 (Debian 1:8.2.2+ds-0ubuntu1.18), `qemu-system-aarch64`. Machines `virt` (Cortex-A53, `-semihosting`) and `raspi3b`. TCG only.
- **Cross compiler:** `aarch64-linux-gnu-g++` 13.3.0 (Ubuntu 13.3.0-6ubuntu2~24.04.1), GNU ld 2.42, `aarch64-linux-gnu-objcopy`, `readelf`, `nm`.
- **Host:** g++ 13.3.0 with `-fsanitize=address,undefined` for host tools (`fdtdump`, `boardscore`, `socsim`); Python 3.13.16.
- **Shared lab sources (in `F4-31`):** `lablib.sh` (`rec`, `kbuild`, `qrun`, `hostbuild`, `hmp`), `start.S`, `kernel.ld`, `kbase.h/.cc`, `fdt.h` (devicetree reader), `minidtc.py` (a small DTS-to-DTB compiler; no `dtc` in the container), `fdtdump.cc`.
- **The kernel:** AArch64, linked at 0 and position independent (`-fpie`, `ld -pie`, R_AARCH64_RELATIVE applied in `start.S`), entered at EL2 or EL1, drops to EL1, MMU off. Only CPU 0 runs. Exits through PSCI SYSTEM_OFF when a `/psci` node is bound, otherwise semihosting SYS_EXIT. Exit code 3 = exception.
- F4-32 builds its own boot-chain stages (ROM, BL2, BL31, BL33) for QEMU virt with `secure=on`; F4-33 builds a UEFI application; F4-34 is a host program only (no QEMU).

## Listings run

**Nothing was run on real hardware.** Every kernel run is QEMU 8.2.2 TCG. Status:

- **pass:** the expected result.
- **expected-fail:** a failure the lab produces on purpose and checks for.
- **untested on hardware:** applies to every QEMU run.

| Chapter | Run (`.log`) | Exit | Status |
|---|---|---|---|
| F4-31 | crosscheck | 0 | pass |
| F4-31 | devices_virt | 0 | pass |
| F4-31 | fdtdump_build | 0 | pass |
| F4-31 | fdtdump_virt | 0 | pass |
| F4-31 | forensic_addr2line | 0 | pass |
| F4-31 | forensic_scan | 3 | expected-fail (exit code 3 = the kernel's exception handler stopped the run); untested on hardware |
| F4-31 | hello_virt | 0 | pass; untested on hardware |
| F4-31 | kbuild | 0 | pass |
| F4-31 | mtree_virt | 0 | pass |
| F4-31 | pci_q35 | 0 | pass |
| F4-32 | boot_ok | 0 | pass; untested on hardware |
| F4-32 | boot_trace | 0 | pass; untested on hardware |
| F4-32 | build | 0 | pass |
| F4-32 | corrupt | 6 | expected-fail (exit code 6 = BL2 refused the image and stopped the run (expected)); untested on hardware |
| F4-32 | corrupt_mkflash | 0 | pass |
| F4-32 | forensic_int | 0 | pass; untested on hardware |
| F4-32 | forensic_serial | 7 | expected-fail (exit code 7 = BL31's handler for an unexpected exception stopped the run); untested on hardware |
| F4-32 | mkflash | 0 | pass |
| F4-32 | nostatus | 124 | expected-fail (exit code 124 = still running after 8 s (no kernel output); kept for Common mistakes); untested on hardware |
| F4-33 | build | 0 | pass |
| F4-33 | forensic_loader | 0 | pass; untested on hardware |
| F4-33 | probe_acpi | 0 | pass; untested on hardware |
| F4-33 | probe_dt | 0 | pass; untested on hardware |
| F4-34 | boardscore | 0 | pass |
| F4-34 | forensic_diff | 0 | pass |
| F4-34 | forensic_new | 0 | pass |
| F4-34 | forensic_old | 0 | pass |
| F4-35 | d8_check | 0 | pass |
| F4-35 | d8_virt | 0 | pass; untested on hardware |
| F4-35 | forensic_dts | 0 | pass |
| F4-35 | forensic_tsens | 0 | pass |
| F4-35 | kbuild | 0 | pass |
| F4-35 | minidtc_soc | 0 | pass |
| F4-35 | socsim_build | 0 | pass |
| F4-35 | socsim_ok | 0 | pass |
| F4-36 | forensic_kernel | 0 | pass; untested on hardware |
| F4-36 | forensic_verify | 1 | expected-fail (exit code 1 = the check failed (this is the evidence)) |
| F4-36 | kbuild | 0 | pass |
| F4-36 | mksd | 0 | pass |
| F4-36 | pinmux | 0 | pass; untested on hardware |
| F4-36 | sd_4096m | 0 | pass; untested on hardware |
| F4-36 | sd_64m | 0 | pass; untested on hardware |
| F4-36 | sd_trace | 0 | pass; untested on hardware |
| F4-36 | verify_4096m | 0 | pass |
| F4-36 | verify_64m | 0 | pass |
| F4-37 | d10_check | 0 | pass |
| F4-37 | forensic_card | 0 | pass |
| F4-37 | forensic_rpi | 0 | pass; untested on hardware |
| F4-37 | forensic_virt | 0 | pass; untested on hardware |
| F4-37 | kbuild | 0 | pass |
| F4-37 | reloc_mistake | 0 | pass; untested on hardware |
| F4-37 | rpi_dtb | 0 | pass |
| F4-37 | rpi_mtree | 0 | pass |
| F4-37 | two_rpi | 0 | pass; untested on hardware |
| F4-37 | two_virt | 0 | pass; untested on hardware |

Values that change from run to run (the output of the F4-32 nostatus run, timer tick counts, RTC readings, busy-wait counter deltas) are not quoted in prose. The F4-37 runs replace RTC readings with `<t>`; the F4-35/F4-37 busy-wait deltas appear only inside output blocks.

## Milestone status

- **D8 (devicetree-driven driver model):** QEMU part done in this build's form (F4-35): every node with a supported compatible on virt is bound by the generic mechanism; the boot log lists bound and unbound nodes; probe ordering works with the consumer before its provider (deferral). Open: the board test (GPIO LED blinking at a timer-set rate, I2C sensor with a logic-analyser trace). No I2C driver and no reset framework in the kernel (resets exist only in the `socsim` model).
- **D9 (SD and eMMC):** PIO SDHCI driver, card identification and initialisation, single-block read and write, on QEMU raspi3b with SDSC and SDHC images, verified from the host (F4-36). Open: ADMA2, eMMC, B14 registration, the `sdhci-pci` configuration with B15/B16, the board tests (boot partition mount, 256 MiB random I/O), card removal.
- **D10 (one kernel, two SoC families):** one file, byte-identical, boots on QEMU virt and QEMU raspi3b; memory, CPUs and timer match (F4-37). Open: a real board from a second SoC family, TF-A, U-Boot's UEFI with the AArch64 A3 loader, B9/B10 on all cores (secondary CPUs are parked), interrupts on raspi3b (no BCM2836 interrupt driver).

## Unverified claims (summary; each chapter has the boxes)

- **No official source was opened.** Every D source is title only (gate G1 open): Devicetree Specification, Linux arm64 booting document, TF-A, U-Boot, SMCCC, PSCI, Arm ARM, UEFI, ACPI, EBBR, SystemReady/BSA/SBSA, SD Physical Layer and Host Controller specifications, JEDEC JESD84, BCM2835 ARM Peripherals, ELF for AArch64, QEMU documentation.
- **From the author's memory, checked only against QEMU's behaviour:** PSCI function IDs and SMCCC layout; SCR_EL3 and HCR_EL2 bits; image header fields; UEFI GUIDs for the devicetree and ACPI configuration tables; SDHCI register offsets and bits; SD command numbers, OCR/CSD field positions and size formulas; the 8-bit shift of 136-bit responses; R_AARCH64_RELATIVE = 1027.
- **Not checked at all:** the 400 kHz identification-clock limit and the SDHCI divider; UHS voltage switching; eMMC initialisation; BCM2835 pin functions and the `brcm,...` compatibles on real hardware; why QEMU's raspi3b loader reduces memory to 960 MiB; why in-section `__rela_*` symbols give a short table; cache state at entry on hardware; board boot flows of the SoC families named in the curriculum.

## Decisions for the owner

1. **Devicetree reader.** DR403 uses its own `F4-31/fdt.h`, not DR402's reader. Merge or keep separate.
2. **Course devicetrees are not vendor trees.** `F4-36/raspi3b.dts` and `F4-35/soc.dts` are written for the course. The `brcm,...` compatibles are unverified; the files must not be used on real boards.
3. **Non-standard bindings.** The pin request properties `pins` (F4-35) and `dr403,sd-pins`, and `/chosen/dr403,sd-selftest`, are course inventions, not the standard pinctrl binding (`pinctrl-0`, `pinctrl-names`). Accept, or rewrite to the standard binding.
4. **SoC model behaviour.** In F4-35's `socsim`, a gated block reads 0 (the forensic "device reads all zeros"). That is a modelling choice; real SoCs may hang, raise an external abort or return stale data.
5. **DT only.** The kernel uses the devicetree, never ACPI; F4-33 discusses ACPI on Arm but no ACPI path was written.
6. **D9 platform.** F4-36 uses QEMU raspi3b's SDHCI, not `sdhci-pci` on virt as D9's first acceptance test says. Decide whether to add the PCI route (the kernel has no PCI on Arm).
7. **D10 substitute.** F4-37 uses QEMU raspi3b as "the second platform" and QEMU's direct kernel boot. It is a different SoC layout but not a board and not firmware.
8. **Fictional boards.** F4-34's bring-up exercise scores fictional boards (no real product data). Real boards are named only inside unverified boxes.
9. **Glossary overlaps.** Not duplicated, only linked: Driver model, Probe (driver) (DR301); Compatible string (DR401); Configuration table (UEFI), GUID, UEFI, ACPI, Reset vector, Memory map (UEFI) and E820 (OS301); Clock tree, Clock gating (clock enable), GPIO, Errata sheet (HW303); GIC, MMIO, PIO, Cache coherence (for DMA) (HW204); Sector and LBA (HW205); Semihosting (OS305); UEFI application (SP301). F4-35's jargon term "Clock gating" and F4-36's "Programmed I/O (PIO)" are not in `glossary.json` because of these. PSCI, SMC and Devicetree are defined here; if DR402 also defines them, one of the two must be removed (duplicate glossary ids). "Devicetree" and "eMMC" replace the curriculum seed entries of the same name.
10. **Diagram types.** Guide 9.2 types used: boot chain timeline (F4-32), driver stack (F4-35).

## Analogy proposals (family F4 — the school and its visitors)

Registered mappings used: firmware = the caretaker; bootloader = the person who unlocks the classroom; driver = an interpreter; interrupt = the doorbell; DMA = the delivery truck. Proposed, for the registry:

- F4-31: devicetree = the building plan handed to a new principal.
- F4-32: boot ROM = the fixed first caretaker at the gate; BL31 = the caretaker who lives in the office; an SMC = a note passed through the office hatch.
- F4-33: SystemReady = a standard school inspection checklist.
- F4-34: TRM = the architect's full drawings; choosing a board = choosing a school by its paperwork before the first visit.
- F4-35: clocks, resets and pins = lights on, door unlocked and furniture arranged before a lesson; deferred probe = the timetable postpones a lesson until the room is ready.
- F4-36: an SD card = a visitor who answers only in a fixed phrasebook, at the counter.
- F4-37: a generic kernel image = a travelling teacher who reads each school's floor plan at the door.

## Forensic labs (injection methods, all in the labs' `run.sh`)

- F4-31: a kernel that scans a fixed address range for devices instead of reading the tree (`blindprobe.cc`); it reads zeros, then takes an external abort at 0x08030ff0.
- F4-32: BL31 built with `-DSCR_VALUE=0x31` (RW bit missing); the ERET to AArch64 EL1 is illegal and the next exception is taken at EL3.
- F4-33: a typo in the devicetree configuration-table GUID (0x41a6).
- F4-34: a deny-list gate that misses a "locked-signed" board (`forensic.in`).
- F4-35 (course forensic "Device reads all zeros"): `clocks = <&ccu 3>` while the clock provider's GATES register enables 0x8.
- F4-36: `block_addressing` forced to false with `sed`; the self-test passes and the host check finds the data inside partition 1.
- F4-37: early console hard-coded at 0x09000000 with `sed`; silent on raspi3b, but the card proves the kernel ran.
