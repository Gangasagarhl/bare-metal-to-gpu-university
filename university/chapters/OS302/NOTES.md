# OS302 — Kernel I: from first instruction to timers: author notes

Chapters F3-18 to F3-25, level L3, 6 credits. Prerequisites: OS301 (or the Limine route), SP301 and SP203.
Labs are in `university/labs/F3-18` to `university/labs/F3-25`. They map to curriculum milestones B1 to B8.

Each lab folder passes `university/labs/run_lab.sh university/labs/<ID>` with status 0. All eight were run again at the end of this build, on 2026-10-09, after the last code change. The numbers in the prose were then checked against the outputs of that final run. The timing figures in F3-25 change from run to run, and the chapter says so.

## Toolchain (as recorded in the logs)

- **QEMU:** emulator version 8.2.2 (Debian 1:8.2.2+ds-0ubuntu1.18), `qemu-system-x86_64`.
  - The `pc` machine with SeaBIOS 1.16.3.
  - **TCG only.** The build container has no KVM.
- **Compiler and binutils:**
  - g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0.
  - GNU ld and binutils 2.42 (objdump, objcopy, addr2line, readelf).
- **Python:** 3.13.16, for QMP helpers and host-clock stamping.
- **Kernel flags** (in `F3-18/kbuild.sh`):
  - `-std=c++20 -ffreestanding -fno-exceptions -fno-rtti -fno-threadsafe-statics -fno-stack-protector -fno-pic -fno-omit-frame-pointer -mno-red-zone -mcmodel=kernel -mgeneral-regs-only -O2 -g -Wall -Wextra -Wpedantic -Werror`.
  - Linked with `ld -nostdlib -static -T F3-18/linker.ld`, then converted to a flat binary with `objcopy -O binary`.
  - The kernel is a flat binary with a Multiboot v1 a.out-kludge header, started with `qemu -kernel`.
- **Host tests:** `-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined`.
- **Shared code:** the kernel accumulates across chapters. F3-19 to F3-25 reuse earlier chapters' sources by relative path, for example `../F3-18/boot.S`. All of them use `F3-18/oslab.sh` for the `rec` log format and `qrun`.
- **QEMU exit codes:** `isa-debug-exit` at port 0xf4. The kernel writes 0x10 for pass, which QEMU reports as exit code 33. It writes 0x11 for fail or panic, which QEMU reports as 35.

## Listings run

**Nothing was run on real hardware, and nothing was run under KVM.** Every kernel run is QEMU 8.2.2 TCG. Every host run is g++ with ASan and UBSan.

Status means:

- **pass:** the expected result.
- **expected-fail:** a non-zero exit that the lab is designed to produce and checks for.
- **untested on hardware:** applies to every QEMU run.

| Chapter | Run (`.log`) | Exit | Status |
|---|---|---|---|
| F3-18 | boot | 33 | pass (B1 kernel prints and exits) |
| F3-18 | assert | 35 | expected-fail (KASSERT panics by design) |
| F3-18 | elfcheck, fmt_host | 0 | pass (host) |
| F3-18 | forensic_boot | 33 | pass (forensic evidence) |
| F3-18 | forensic_diff, forensic_elf | 0 | pass (forensic evidence) |
| F3-19 | desc_host | 0 | pass (host) |
| F3-19 | exceptions, intlog | 33 | pass |
| F3-19 | forensic_serial | 0 | expected: a triple fault resets the guest; `-no-reboot` ends QEMU with 0 |
| F3-19 | forensic_int | 0 | pass (QEMU `-d int` evidence) |
| F3-20 | pmm_host | 0 | pass (host) |
| F3-20 | mem_128M, mem_1G, mem_4G | 33 | pass |
| F3-20 | forensic_leak | 35 | expected-fail (leaking driver exhausts memory, panic) |
| F3-21 | pte_host | 0 | pass (host) |
| F3-21 | b4, walk | 0 | pass (the QMP helper quits QEMU after its monitor commands) |
| F3-21 | guard, forensic_tlb, forensic_fixed | 33 | pass |
| F3-22 | heap_host | 0 | pass (host, ASan) |
| F3-22 | b5 | 33 | pass |
| F3-22 | uaf | 35 | expected-fail (poison check names `kmalloc-64`) |
| F3-22 | forensic | 35 | expected-fail (name-table overrun, poison report) |
| F3-23 | boot, screen, panic_screen, forensic_monitor | 0 | pass |
| F3-23 | smbios | 35 | expected-fail (banner kernel then panics by design) |
| F3-23 | panic_lock | 35 | expected-fail (panic while holding the log lock completes) |
| F3-23 | forensic_serial | 124 | expected-fail (hang stopped by the 15 s time limit) |
| F3-24 | boot, x2apic_on, x2apic_off, forensic_serial, forensic_monitor, madt_host | 0 | pass |
| F3-24 | corrupt | 33 | pass (damaged MADT rejected) |
| F3-25 | clock_host | 0 | pass (host) |
| F3-25 | hpet, pit | 33 | pass; the TCG host-clock difference is within 1 % |
| F3-25 | forensic | 33 | pass as a run; its host-clock verdict is "OUTSIDE tolerance" by design |

## Acceptance tests B1–B8 and their honest status

The acceptance tests for B1 to B4 are recorded in the chapters for F3-18 to F3-21. All of them pass under QEMU TCG.

**B5 (F3-22):** all three tests pass.

**B6 (F3-23):** all three tests pass.

- The screenshot test is pixel-exact, with tolerance 0.
- The reference image is rebuilt from the serial log by the same console code, not stored.

**B7 (F3-24):**

- Tests 1 and 2 pass.
- Test 3, "the same kernel works with x2APIC on and off", passes as worded. However, **the x2APIC code path never ran.** QEMU 8.2.2 TCG prints "TCG doesn't support requested feature: CPUID.01H:ECX.x2apic" and still reports no x2APIC, so both runs used xAPIC.
- The XSDT path of `acpi::init` never ran either. SeaBIOS gives a revision-0 RSDP on both `pc` and `q35`.
- The host test `madt_host` covers the x2APIC MADT entry (type 9). It does not cover the MSR path.

**B8 (F3-25):**

- **Test 1:** the TCG result is logged: −0.00 % with the HPET and +0.05 % with the PIT fallback in the final run. **The KVM measurement with a 1 % tolerance was not run.**
- **Test 2:** 0 backwards in 10,000,000 reads, on one CPU only. The all-CPU version belongs to B11 (OS303).
- **Test 3:** the HPET-off fallback to the PIT passes.

## Unverified boxes (one per chapter; all sources are "title only", dossier gate G1 open)

**F3-18**

- Multiboot v1 header fields, flags, magic values and the info layout (D2).
- The CR0, CR4 and EFER bits, the descriptor constants and PDE 0x83 (D1).
- The 16550 offsets and divisor (D4).
- The `isa-debug-exit` formula (D6). The two observed exit codes confirm it.

**F3-19**

- Which vectors push an error code, and the exception names.
- The gate, TSS and TSS-descriptor layouts.
- The page-fault error-code bits.
- The canonical-address rule.
- The double-fault combination table.

Source: Intel SDM Vol. 3 (D1) and AMD APM Vol. 2 (D2).

**F3-20**

- The Multiboot memory-map entry layout and type values (D2).
- E820 and UEFI memory types (D9, D3).
- The QEMU reserved range at 0xfd00000000 (D10).

**F3-21**

- EFER.NXE, CR0.WP and CR4.PGE.
- The PAT MSR 0x277 and its encodings.
- The PAT bit positions.
- The rights-combination rule and global-entry behaviour.

Source: D1 and D2.

**F3-22**

- The exact list of replaceable allocation functions, and which forms the compiler calls (C++ [new.delete] and [expr.new], D2).
- That `std::nothrow` lives in libsupc++. This is inferred from the link error.
- The description of Linux SLUB debugging and KASAN (D3).

**F3-23**

- The DISPI ports and registers, and the PCI id 1234:1111. These are QEMU device details.
- The SMBIOS entry-point and structure offsets (D3).
- PAT WC = 0x01 (D4).
- The UEFI GOP pixel formats (D2).
- The 16550 timing figure (D1).

**F3-24**

- The RSDP layout and search areas.
- The FADT DSDT offset 40.
- The MADT entry types and offsets, and the override flags (D1).
- The IA32_APIC_BASE bits.
- The local APIC register offsets and the x2APIC MSR mapping (D2).
- The IOAPIC registers and redirection bits (D3).
- The 8259 initialisation words (D4).

The x2APIC and XSDT paths are untested.

**F3-25**

- The HPET registers and capability bits, the 100 ns limit, and the ACPI HPET address offset 44 (D1, D4).
- The PIT ports, command byte 0xB0, mode 0 and 1,193,182 Hz (D3).
- The APIC timer registers and divide value (D2).
- The invariant-TSC bit.
- The `sti` interrupt shadow.
- The TSC-deadline CPUID bit, which is mentioned only in an extension.

## Decisions for the owner

1. **Boot route.** The curriculum's B1 assumes UEFI with Limine, the A3 route. Neither Limine nor OVMF was available in the container, so OS302 boots with **Multiboot v1 through QEMU's `-kernel`**. The kernel is a flat binary with an a.out-kludge header, and its 32-bit entry code switches to long mode itself. This also means:
   - F3-23 has no GOP framebuffer. It programs QEMU's `-vga std` (Bochs DISPI) instead, which works only under emulation.
   - F3-24 finds the RSDP by a memory scan, not from the UEFI configuration table.
   - Please confirm this route, or schedule a Limine/UEFI variant.
2. **TCG only.**
   - Every timing number comes from TCG: the F3-25 sleeps, calibrations and monotonic-read costs, and the F3-22 stress-test duration.
   - The B8 KVM 1 % test is **not run**.
   - Decide whether B8 test 1 counts as passed with "TCG logged, KVM pending".
3. **x2APIC untested.** B7 test 3 passes as worded, but the x2APIC branch never executed. The same applies to the XSDT path. Running B7 needs a KVM host, or a QEMU version whose TCG implements x2APIC.
4. **Own font.** F3-23 draws its own 5×7 font, `font5x7.h`, by hand. Lower-case letters use the upper-case shapes. No third-party font is included, so there is no licence question. Replace it if a fuller font is wanted.
5. **Simplifications to carry into OS303.** Each chapter states these:
   - Page-table pages are never freed (F3-21).
   - The heap's large region uses a bump pointer, so freed virtual ranges are not reused (F3-22).
   - The heap has no lock and no per-CPU caches (F3-22).
   - The panic path does not stop other CPUs (F3-23).
   - The timer queue is a sorted array of 32 entries (F3-25).
6. **Shared files across chapters.** Later chapters compile earlier chapters' sources by relative path. Examples are `F3-18/arch.h`, `F3-18/kprint.cc` and `F3-23/log.cc`. Helpers were added to `F3-18` during later chapters:
   - `%.Ns` precision in `kformat.h`;
   - `sti_hlt`, `pause`, `save_flags_cli` and `restore_flags` in `arch.h`.

   All eight labs were rerun after these changes.
7. **Glossary.**
   - `glossary.json` has 70 entries.
   - Terms that already exist in other courses reuse their exact names so that build.py merges them: "Kernel (operating system)", "Freestanding implementation", "Exception", "Interrupt", "Page table", "Memory leak", "Use after free" and "Deadlock".
   - The F3-22 jargon box spells "Use-after-free". The glossary entry uses "Use after free", the same term as SP201, so the anchor `#gl-use-after-free` is shared.
8. **An open forensic detail.** In the F3-24 no-EOI run, QEMU's `info irq` counted 4 raises of IRQ 4, but the kernel logged 3 serial interrupts. The chapter does not explain the extra one, and says so. It does not affect the diagnosis.

## Build check

- `python3 university/build/build.py` ran with exit status 0.
- None of its PROBLEM lines refer to OS302. All of them are broken `#gl-` links from other courses' chapters.
- Every `#gl-` link in F3-18 to F3-25 resolves to an entry in this course's `glossary.json`.
