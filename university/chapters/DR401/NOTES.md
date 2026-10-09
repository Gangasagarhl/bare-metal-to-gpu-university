# DR401 — Any hardware: the universal method and real machines: author notes

Chapters F4-14 to F4-22. Level L4, 5 credits. Prerequisite DR302 (C1–C9).
Maps to curriculum sections 5 and 6, milestones M1–M4, C12, C14 and C15, and runbook 19.5.
Labs are in `university/labs/F4-14` to `university/labs/F4-22`.

## Build state

**Labs.** Each of the nine labs passes `university/labs/run_lab.sh university/labs/<ID>` with exit code 0. All nine were re-run at the end of this build (2026-10-09).

**Prose against the last run.** The prose was then checked against the outputs of that last run. Numbers that change between runs are given as "in the saved run", and the text says that they vary. Where an earlier run's numbers are quoted, the text says that its log was not kept. These numbers are:
- poll counts and access totals (F4-16, F4-17, F4-18);
- stage ticks (F4-20);
- host CPU percentages and timer intervals (F4-22).

**Fragment checks.** Every fragment passes the local checks:
- html balance;
- ids prefixed with the chapter id;
- no URLs;
- no `<script>`;
- all 21 sections in order, plus Answers and the forensic key;
- every `data-src` and `data-run` file exists;
- source anchors resolve;
- at least one inline SVG figure;
- no banned words.

**build.py.** `python3 university/build/build.py` prints no PROBLEM line that mentions DR401 or F4-14 to F4-22. None of its broken-link lines is a link from these chapters. The remaining PROBLEM lines (broken `#gl-` links) belong to other courses.

**Glossary.** `glossary.json` has 56 four-part entries, generated from the chapters' jargon boxes. Every `#gl-` link in the nine chapters resolves to one of them.
- One rename was needed to avoid an id collision. F4-18's "Shim" is now "Test shim", because OS301 defines "shim" (the UEFI boot shim) with the same anchor.
- Three terms share an exact name with other courses: Boot trace (DR301), Report descriptor (OS305) and SMBIOS (OS301, OS302). build.py merges entries with the same name. The definition that appears is the first course's in alphabetical order, so DR301's for Boot trace and ours for the other two.

## Shared code between labs

**F4-14 holds the shared kernel code.** The files are:
- `boot.S` (Multiboot 1, 32-bit);
- `k4.*`, `pci4.*`, `acpi4.*`, `kernel.ld`;
- the helper `dr401lib.sh`, which provides `rec`, `kbuild`, `hostbuild`, `qboot` and the QEMU options.

**Later labs build on earlier ones by relative path:**
- F4-15's `bind4.*` is used by F4-20.
- F4-17's `edu4.*` is used by F4-18.
- F4-19 reads F4-14's outputs.

Do not move one folder without the others.

**Toolchain** (recorded in each `.log`): g++ 13.3.0, GNU ld 2.42, Python 3, QEMU 8.2.2 with TCG (no KVM in the container).

**`.cc` naming.** Multi-file host programs are named `.cc` and built in `run.sh`. `run_lab.sh` compiles every `.cpp` standalone, so these sources would otherwise break the lab.

## Listings run

**Nothing was run on real hardware.** Every QEMU run is untested on hardware, and its log says so.

**F4-14**
- pass: build; devreport (exit 33); acpi_qemu; qmp_compare; inventory; names; acpi_ids; cpuid_id; fdt_virt.
- forensic: exit 33, expected.
- forensic_compare: exit 1, expected (the lists differ).

**F4-15**
- pass: build; boottrace (exit 33); tier; docs_local; forensic_qtree.
- forensic_hwids: the Windows ID scheme is assumed.
- forensic_inf: the INF exhibit is constructed.

**F4-16**
- pass: build; observe (exit 33); trace; annotate.
- forensic_run: exit 35, expected.
- forensic_diff: exit 1, expected.

**F4-17**
- pass: build; driver (exit 33); regs_doc_check.
- removal: exit 35, the expected clean result.
- forensic: exit 35, expected.

**F4-18**
- pass: unit; replay; mutation; golden_trace (exit 33); soak (exit 33).
- absent: exit 35, expected (test E2).
- forensic_ci: exit 0, which is itself the evidence.
- forensic_field: exit 35, expected.

**F4-19**
- pass: machines; catalog; qemu_devices; forensic (exit 0, 19 unclassified, the evidence).

**F4-20**
- pass: build; boot (exit 33); boot_nops2 (exit 33); timeline (exit 0); forensic_boot (exit 33, slow).
- forensic: exit 1, expected.
- The real-PC steps are untested: no PC, and no grub-mkrescue or xorriso to make a USB image.

**F4-21**
- pass: extract; touchpad (exit 0, against a software model).
- hid_dump: exit 1, expected (the self-check flags the keyboard descriptor).
- forensic: exit 1, expected.
- The laptop is untested.

**F4-22**
- pass: build; idle_halt (exit 33); idle_poll (exit 33); forensic (exit 33, the evidence); power_readout (exit 0); governor; thermal.
- The spare-PC part of C15 is untested.

**Predict-then-run answers.** Every "predict, then run" answer was checked by an actual run in a scratch copy:

| Chapter | Change made | Result |
|---|---|---|
| F4-14 | edu removed | 9 functions, identical |
| F4-15 | 00:1f.3 set to unknown | the tier counts given in the answer |
| F4-16 | input 12 | 0x1c8cfc00; 173 accesses in that run |
| F4-17 | REMOVE_AT=0 | "computes 0 polls 1 timeouts 0 gone 1" |
| F4-18 | busy_polls 0 | only T9 fails, 8 of 9 |
| F4-19 | cfi-flash key added | 11 unclassified |
| F4-20 | timeline boot vs forensic_boot | ratio 2974.7, exit 1 |
| F4-21 | stray 0x06 removed | 64 bits = 8 bytes, 0 problems |
| F4-22 | latency_limit_us 100 | C1 21, C3 39; 13551.9 µJ; 1992 µs |

## Unverified boxes (by chapter)

Every chapter cites its documents as title only (dossier gate G1 open). Each chapter also has an "untested on hardware" box.

**F4-14**
- AML encodings and the EISA ID compression.
- The PCI CONFIG_ADDRESS layout and "all ones when absent".
- CPUID family and model formula, and the hypervisor leaf.

**F4-15**
- The Windows hardware-ID list format of the forensic exhibit.
- Firmware switching a SATA controller's programming interface.
- No search was possible, so the "public" fields rest on the catalog.

**F4-16**
- The legal texts were not opened. This is not legal advice.
- mmiotrace and VFIO trapping internals, and the QEMU trace format.
- USB, logic-analyser and e1000-under-Linux observation were not run.

**F4-17**
- All-ones reads after surprise removal (not observed).
- PCI posted-write ordering and x86 uncached MMIO ordering.

**F4-18**
- edu computing in a delayed thread (the reason poll counts vary).
- Automatic mutation tools.

**F4-19**
- Catalog keys and first documents come from the curriculum only.
- The milestone column is partly the author's mapping.

**F4-20**
- SMBIOS and Multiboot offsets were written from memory. The checksum and the `-smbios` cross-check pass.
- i8042 status bit 1, and "absent port reads 0xFF" (QEMU's behaviour; on real hardware it depends on the chipset).
- The FADT 8042 flag.
- The UEFI location of the SMBIOS entry point.

**F4-21**
- HID item tags, usage numbers and the unsigned-maximum rule.
- The QEMU keyboard descriptor anomaly (see decision 3).
- The whole HID-over-I2C protocol layout: register order, opcodes RESET=1 and SET_POWER=8, the zero-length reset answer, the 2-byte length prefix. The model and the driver come from the same memory.
- Linux's -ENXIO for an address NACK.

**F4-22**
- PIC and PIT programming, the gate type, and the `sti` one-instruction shadow.
- CPUID leaf 6 bit meanings (8 of 8 agree with `/proc/cpuinfo`, but only one bit is set).
- Invented teaching constants in `governor.cpp` and `thermal.cpp`.

## Decisions for the owner

### 1. Tier list inconsistency (F4-15)

**Issue.** The curriculum's tier list in section 5 does not match the 5.2 table. F4-15 Layer 3 notes this.

**Current handling.** The course uses the 5.2 table, and "signed firmware" is mapped to tier 6 or to the host-interface tier.

**Owner's call.** Confirm the mapping, or fix the curriculum.

### 2. edu device tier (F4-15, F4-16)

**Issue.** The F4-15 forensic answer classifies edu as tier 2: the QEMU project is its maker, and its documentation exists but was not opened. F4-16 deliberately treats it as tier 5 for practice and says so.

**Owner's call.** Confirm that this pedagogical choice is acceptable.

### 3. QEMU USB keyboard descriptor (F4-21)

**Issue.** The bytes extracted from the QEMU 8.2.2 program file contain "95 06 06 75 08". With the extra 0x06, the input report is 34 bits. Without it, the report is 64 bits.

**Status.** Unresolved: the build had no guest USB stack to check what QEMU actually sends. The chapter says not to cite it as a QEMU bug. The incomplete mouse candidate at file offset 13113712 is also unexplained.

**Owner's call.** Have a Linux guest dump of the descriptor taken, then update the chapter. The `hid_dump` run is designed to exit 1 on this descriptor, so `run.sh` would need changing if the bytes turn out to be wrong.

### 4. Zero reads with memory decoding off (F4-17, F4-18)

**Observation.** In this build, QEMU returns 0, not all ones, for reads with memory decoding off. The curriculum says "all ones" after removal.

**Current handling.** The driver checks the ID register instead, which works for both values.

**Owner's call.** Verify against the PCI specification and real hardware.

### 5. Constructed and assumed exhibits (F4-15)

**Issue.** The INF excerpt is constructed (labelled CONSTRUCTED in the log). The Windows hardware-ID list format follows the curriculum's forms but is unverified.

**Owner's call.** Accept, or supply a real (licence-clean) exhibit.

### 6. Register tables cite undocumented sources (F4-17)

**Issue.** M2's third acceptance test wants every register to cite a document section. F4-17's register table cites F4-16's observed specification and the unopened QEMU documentation.

**Owner's call.** This needs the Source Researcher (gate G1).

### 7. Invented constants (F4-22)

**Issue.** The governor and thermal models use invented constants. They are labelled in the code, the outputs and an unverified box.

**Owner's call.** Accept as teaching models, or replace them with platform values when a spare PC is available.

### 8. Timing-dependent outputs

**Issue.** Several labs depend on timing:
- F4-16 trace totals ranged from 141 to more than 3600 accesses across this build's runs;
- F4-17 and F4-18 poll counts;
- F4-20 stage ticks;
- F4-22 CPU percentages and timer intervals, whose scatter under TCG reversed between two runs.

Each rebuild of the outputs can change the quoted numbers.

**Owner's call.** Either accept the "saved run" wording, or make these labs deterministic. QEMU's `-icount` would do that, but it changes the halt/CPU-time measurement in F4-22, which depends on real host time.

### 9. Course-level items not written as separate files

**Coverage.** The course forensic ("Unknown device": hardware IDs, an INF excerpt, a boot trace → device record and tier) is F4-15's forensic lab. The project M2 is carried by F4-17 and F4-18.

**Not written.** The exams (Q, F, P) were not asked for as files and were not written.

**Owner's call.** Say whether they are wanted.

### 10. Real hardware

No real hardware was available. C12, C14 and C15 are complete only in their QEMU, model or host parts, and each chapter's Verification paragraph says which acceptance lines remain.

## Analogy notes

All chapters use the F4 entry, "the school and its visitors": a driver is the interpreter for a visitor. Additional roles come from the same F3/F4 table and keep its meanings:
- the principal (the OS);
- the doorbell (an interrupt);
- the caretaker (firmware).

In particular, F4-22's idle governor is the receptionist and the thermal policy is the principal's. The caretaker appears only as the firmware that also reacts to heat.

F4-21 adds a bell board (GPIO interrupt routing) and a corridor runner (the I2C controller). These are new images, but they do not conflict with the table. The owner may want to add them to the guide's F4 table:

| Concept | Analogy | Where it breaks |
|---|---|---|
| I2C bus | A corridor with numbered rooms and a runner who knocks | — |
| GPIO interrupt | The office's bell board | A level-triggered line keeps ringing until the cause is cleared |

Every chapter has a "Where the analogy breaks" section.
