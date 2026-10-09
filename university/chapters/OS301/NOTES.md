# OS301 — Firmware and boot: author notes

Chapters F3-09 to F3-17, level L3. F3-17 is optional (milestone A4).
Labs are in `university/labs/F3-09` to `university/labs/F3-17`.

Each lab folder passes `university/labs/run_lab.sh university/labs/<ID>` with status 0. All nine were run again at the end of this build, on 2026-10-09 (see "Final run" below).

## Toolchain and local evidence (as recorded in the logs)

- **Host compiler:** g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0.
  - Course flags for host programs: `-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined`.
  - Kernel flags (F3-14, F3-15): `-std=c++20 -ffreestanding -fno-exceptions -fno-rtti -nostdlib -fno-pic -fno-pie -mcmodel=kernel -mno-red-zone -mgeneral-regs-only -fno-stack-protector -O2 -Wall -Wextra -Wpedantic -Werror`, linked with `ld.lld -T kernel.ld -z max-page-size=4096`.
- **UEFI programs:** Ubuntu clang 18.1.3 with `--target=x86_64-unknown-windows -ffreestanding -fno-exceptions -fno-rtti -mno-red-zone -fno-stack-protector -O2 -Wall -Wextra -Wpedantic -Werror`, then `lld-link /subsystem:efi_application /entry:efi_main /nodefaultlib` (LLD 18.1.3). F3-16 links with `/Brepro` so that its digests are the same on every run.
- **Legacy boot sector (F3-10, A5):** nasm, as recorded in `F3-10/build.log`.
- **QEMU:** 8.2.2 (Debian 1:8.2.2+ds-0ubuntu1.18), q35 machine.
- **Firmware:**
  - OVMF from the ovmf package 2024.02-2ubuntu0.10: `OVMF_CODE_4M.fd`, `OVMF_CODE_4M.secboot.fd` (the `.ms` and `.snakeoil` code files are symbolic links to it), three variable stores (plain, `.ms`, `.snakeoil`), `OVMF.fd`, `OVMF.amdsev.fd`.
  - SeaBIOS 1.16.3-2.
  - All of these are release builds as far as the debug port shows. The packaged OVMF writes 0 bytes to port 0x402 (`F3-17/debugcon`).
- **Other tools:**
  - GNU Binutils 2.42 (readelf, objdump, strings).
  - gdb (F3-09, through QEMU's gdb stub).
  - sgdisk, mkfs.fat and mtools (F3-13).
  - llvm-readobj 18.1.3.
  - Python 3.13.16 with python3-cryptography 50.0.1 (only the RSA operation in `F3-16/sign.py`).
  - GNU coreutils sha256sum 9.4.
- **Not available:**
  - No real PC, no TPM (no swtpm), no internet.
  - No EDK II or coreboot source tree, no `build` or `iasl` command (`F3-17/toolcheck`).
  - No sbsigntools, no OVMF variable-store tool.
  - No Linux disk image.
- **Local files opened as evidence:** the ovmf package's test key `/usr/share/ovmf/PkKek-1-snakeoil.pem` and `.key`. The key's passphrase "snakeoil" was guessed and accepted; no file in this build documents it.

## Listings run

**Nothing was run on real hardware.** Every QEMU run is marked "untested on hardware" in its log.

| Chapter | Run | Status |
|---|---|---|
| F3-09 | vector, reset_bytes, gdb_reset, walk | pass (walk: the hand page walk equals QEMU's gva2gpa) |
| F3-09 | healthy_console | pass (exit 124 = the harness's time limit, by design) |
| F3-09 | forensic_console, forensic_bytes, forensic_gdb | forensic evidence: firmware with its last 16 bytes zeroed. `forensic_console` is an **expected fail** (no output, exit 124) |
| F3-10 | build, bootsect_image, bootsect (exit 33), healthy_seabios | pass |
| F3-10 | memmap_256, memmap_512, check_a1 | pass as runs. **A1's literal acceptance test FAILs** (see "Decisions for the owner") |
| F3-10 | forensic_seabios, forensic_serial | forensic evidence: a boot sector without the 55AA signature. **Expected fail** (exit 124) |
| F3-11 | fvscan, sample, lastfile | pass |
| F3-11 | forensic_fvscan, forensic_cmp, forensic_sample, forensic_console | forensic evidence: damaged firmware volume. **Expected fail**: `forensic_console` exits 124 with no output |
| F3-12 | tables (exit 33), check_tables | pass |
| F3-12 | timeline_healthy, timeline_slow | pass: three boots each. `timeline_slow` is forensic evidence (splash-time variable) |
| F3-13 | mkimage, sgdisk, gptcheck, boot_gpt | pass |
| F3-13 | corrupt_check | **expected fail**: exit 1, the verifier detects the corrupted primary header |
| F3-13 | corrupt_boot, corrupt_after_boot | pass: OVMF boots from the backup header and repairs the primary |
| F3-13 | forensic_check, forensic_sgdisk, forensic_boot, forensic_fixed | forensic evidence: an image grown to 192M. `forensic_check` is an **expected fail** (exit 1); `forensic_fixed` passes after `sgdisk -e` |
| F3-14 | kernel_build, elfinfo, elfattack (9 named + 200,000 random cases) | pass |
| F3-14 | multiboot | pass (exit 33) |
| F3-14 | forensic_qemu | **expected fail**: exit 1, "Error loading uncompressed kernel without PVH ELF Note" |
| F3-14 | forensic_notes, forensic_mbclass | pass (evidence) |
| F3-15 | build, a3 | pass: **A3 acceptance 10 of 10** |
| F3-15 | bad_badmagic, bad_overlap | pass: the loader rejects the kernel and returns to the firmware (exit 124 = harness stop) |
| F3-15 | forensic_stale2 | **expected fail**: ExitBootServices fails with 0x8000000000000002 and the planted loader spins (25 s limit) |
| F3-15 | forensic_stale1, forensic_healthy | pass. `forensic_stale1` was planned as a failure but booted (see F3-15 notes) |
| F3-16 | sb_plain, sb_nokeys (exit 33) | pass |
| F3-16 | sb_enrolled, sb_snakeoil_unsigned | **expected fail**: "Access Denied" (exit 124) |
| F3-16 | signature, sign, sb_snakeoil_signed (exit 33, SecureBoot = 1) | pass |
| F3-16 | sb_snakeoil_tampered | **expected fail**: forensic evidence, "Access Denied" |
| F3-16 | measure, crosscheck, forensic_hashes, repro | pass |
| F3-17 | packages, diff_secboot, diff_amdsev, seabios, toolcheck, debugcon | pass |
| F3-17 | forensic_mixed | pass (exit 33): forensic evidence, the PK is present but Secure Boot is not enforced |

Untested on hardware: every chapter. Untested at all: the A4 build plan (F3-17), the TPM (F3-16), the USB-stick boots of A1 and A2, and the "boot a Linux image" step of P3 (F3-09).

## Unverified claims (boxes "Not verified — check before relying on this")

Every D-source is cited by title only (dossier gate G1 open). Each chapter has three to five boxes.

- **F3-09**
  - The purpose of the reset-vector instructions (CR0.PE test, DI = "BP", ESP as scratch).
  - The hardware power-on sequence: security processor, microcode, flash bus, DRAM training.
  - P3's Linux step: not done.
- **F3-10**
  - The layouts in `efi.hpp`: tables, slot orders, GUIDs.
  - BIOS service numbers and conventions (INT 13h, INT 15h E820) and the 16550 registers.
  - Untested on hardware: USB and CSM.
- **F3-11**
  - Firmware-volume, FFS and section offsets and type numbers.
  - OVMF internals not checked in EDK II: SEC decompression, and the 0x820000 range marked ACPI NVS.
  - Vendor images.
- **F3-12**
  - ACPI and SMBIOS field offsets.
  - FPDT content and its use by ETW.
  - Real-table quirks.
- **F3-13**
  - sgdisk, mkfs.fat and mtools options.
  - GPT and FAT32 field offsets.
  - The USB boot (A2 optional item).
- **F3-14**
  - ELF and psABI field offsets.
  - Page-permission semantics.
  - Limine and Linux boot protocol descriptions (taken from the curriculum's table).
  - Multiboot constants.
- **F3-15**
  - The rule for boot-service calls after a failed ExitBootServices. Listing 1's give-up path and the forensic build print through ConOut after a failure.
  - The EFI status values.
  - PTE bits, EFER.NXE and the stack-alignment rule.
  - Real-firmware differences.
- **F3-16**
  - Variable names, GUID and attributes.
  - The contents of the `.ms` and `.snakeoil` stores (not listed).
  - All structures in `sign.py`: it is accepted by OVMF, not checked against the specifications.
  - PCR assignments, Authenticode measurement and the event log.
  - Boot Guard and PSB.
  - The EFI_SIGNATURE_LIST layout.
- **F3-17**
  - Module roles, which are inferred from their names.
  - The purpose of the AMD SEV build.
  - The whole A4 plan, which is untested.
  - SMRAM, flash protection and port 0x80.

## Decisions for the owner

1. **A1's literal acceptance test fails (F3-10).** Read literally, with conventional memory only, it fails. Conventional memory is 48.9 MiB below the `-m` value at both 256 and 512 MiB, because the firmware, the shell and the program hold boot-services and loader memory while A1 runs (`check_a1`).
   - Read as "memory usable after ExitBootServices", the test passes, 6.2 MiB below at both sizes.
   - The 6,368 KiB the firmware keeps for good is runtime 3,348 KiB, ACPI 2,124 KiB, reserved 512 KiB and the VGA hole 384 KiB.
   - The chapter reports both readings. The owner should choose one and reword the curriculum's test.
2. **No FPDT in this OVMF (F3-12).** The boot timeline uses host time stamps plus the TSC, three boots per configuration. FPDT content is left in an unverified box. Should a later edition use a debug or performance build of OVMF?
3. **P3's "boot an existing Linux image and break inside it" (F3-09)** was not done: there is no image and no internet. The page walk was done on the firmware's own page tables. Provide an image in the lab environment, or drop the step.
4. **A2's USB-stick boot (F3-13) and A1's USB run (F3-10)** are optional items. They are untested on hardware.
5. **A3's "as its first act, writes a known pattern to the framebuffer" (F3-15).**
   - The kernel stub prints "kernel entered" first and paints the framebuffer last, so the serial port shows progress even if the framebuffer address is wrong.
   - The acceptance test checks the painted pattern, not its order.
   - Accept this, or reorder `kernel.cc` (F3-14 lab, lines 171–176) to paint first.
6. **A3's direct map** covers max(top of RAM, 4 GiB) with 2 MiB pages. It was only exercised with 256 MiB of RAM.
7. **A4 (F3-17) cannot be built here.**
   - The chapter does what is possible and gives the build as an untested plan: it compares three packaged OVMF builds module by module, and compares the debug-port output of SeaBIOS and OVMF.
   - To complete A4, a lab image needs the EDK II and coreboot trees, iasl (acpica-tools) and network access for submodules, or prepared tarballs.
8. **F3-16 deviates from the curriculum's exercise.**
   - The exercise is "enroll your own PK/KEK/db in OVMF and sign your loader (sbsigntools)". sbsigntools and a variable-store tool are not installed.
   - The lab instead uses the distribution's pre-enrolled snakeoil store. It signs with a hand-written Authenticode signer (`sign.py`), which OVMF accepts.
   - Install sbsigntools and python3-virt-firmware (or efitools) so the original exercise can be done. `sign.py` should then be cross-checked against `sbverify`.
9. **F3-15 forensic variant 1.** A 64-byte AllocatePool between GetMemoryMap and ExitBootServices did *not* invalidate the map key on this OVMF. Variant 2, AllocatePages, did. Both are kept, and the chapter explains the difference as a hypothesis (pool served from an existing page).
10. **Verbatim curriculum quotes that contain banned words.**
    - "just" in P3's text (F3-09).
    - "trivial" in A4's acceptance text (F3-17).
    - They are kept verbatim inside quotations. Reword the curriculum, or accept them as quotes.
11. **Chapter length.** The chapters run 4,900 to 6,600 prose words, against a target of about 4,000 for L3. The extra length is mostly in the lab, forensic and answer sections. They could be trimmed if the owner prefers.
12. **F3-17 worked example.** It states that one driver and one PEIM name differ in count between the plain and AMD SEV builds, without naming them; finding them is a lab extension. Answer for the instructor: `CpuDxe` (DRIVER, 2 against 1) and `CpuMpPei` (PEIM, 2 against 1). This was checked with fvscan during authoring, not in a recorded run.

## Analogy proposals (school, F3)

Registered mappings used: firmware = the caretaker; bootloader = the person who unlocks the classroom; operating system = the principal and the timetable; page table = the map of lockers; file system = the library catalogue and shelves.

| Proposal | Chapter |
|---|---|
| boot services = the caretaker's tools on loan until the principal takes over | F3-10 |
| PI phases = the caretaker's morning round: front door, boiler room, every classroom, the timetable board | F3-11 |
| ACPI and SMBIOS tables = the building's inventory folder the caretaker leaves on the principal's desk | F3-12 |
| partition table = the building's room plan, kept twice: one copy at the entrance, one at the back door | F3-13 |
| boot protocol = the handover checklist the two sign | F3-14 |
| map key = the stamp on the room plan; the caretaker only leaves if the plan you show him carries today's stamp | F3-15 |
| Secure Boot = the caretaker checks a signed permission slip against the list on the staff-room wall | F3-16 |
| measured boot = the visitors' book, where each visitor's name is added and nobody can tear out a page | F3-16 |
| a firmware build = writing the caretaker's job description: which rooms he opens, which keys he carries, whether he keeps a diary | F3-17 |

## Glossary

`glossary.json` has 58 four-part entries, taken from the chapters' jargon boxes, plus "Reproducible build".

- Some terms already exist with the identical term string, and are merged by build.py: Firmware, ELF, Long mode, ACPI, MADT, SMBIOS, Higher-half kernel. Their definitions come from those courses' glossary files, OS302 and SP301 among them.
- Seed terms such as OVMF, UEFI, CRC32 and ExitBootServices are written in four parts here. The seed entries keep their one-line definitions without an id.
- Page table and File system are linked to existing entries (HW203, OS201, OS302) and are not redefined.

## Final run

All nine labs were run again with `run_lab.sh` at the end of this build, on 2026-10-09. All nine returned status 0.

Some runs depend on timing or sampling, so their outputs differ from run to run:

- F3-11: sample, forensic_sample.
- F3-12: tables (the TSC value), timeline_healthy, timeline_slow.
- F3-13: boot_gpt, corrupt_boot, forensic_boot (extra screen lines were captured).
- F3-15: a3, forensic_healthy, forensic_stale1, forensic_stale2 (TSC values and host time stamps).
- F3-16: repro (the time stamps of the links made without /Brepro).

For these 13 runs, the lab folders keep the `.out` and `.log` of the earlier passing run that the prose quotes. Those earlier runs also passed `run_lab.sh`. The final run's outputs had the same verdicts but different numbers, and they were not kept. F3-11 says in its prose that sampling is not repeatable, and gives the final run's values as a comparison.

All other outputs of the final run are kept, and they are byte-identical to the outputs the prose quotes.
