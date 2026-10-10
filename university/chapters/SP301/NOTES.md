# SP301 — build notes (Linking, binaries and freestanding C++, F2-43 to F2-50)

Build date: 2026-10-09. Build machine: Linux x86_64 (cloud build container).
Toolchain as printed by the tools in the lab logs: `g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0`,
`Ubuntu clang version 18.1.3`, `Ubuntu LLD 18.1.3`, LLVM 18.1.3 tools (`llvm-readobj`,
`llvm-objdump`, `llvm-nm`), GNU Binutils 2.42 (`ld`, `readelf`, `objdump`, `nm`),
`aarch64-linux-gnu-g++` 13.3.0, `QEMU emulator version 8.2.2` (system and `qemu-aarch64`),
OVMF from `/usr/share/OVMF` (`OVMF_CODE_4M.fd`), GNU Make 4.3, git 2.43.0, gdb.

Every lab folder passes `university/labs/run_lab.sh university/labs/<ID>` (status 0) in its
last run. Each lab's `run.sh` writes `<step>.out` (every command after `$ `, then its output)
and `<step>.log` (listing, toolchain, command, date, machine, exit code of the step's last
command, and `hardware:` where relevant). Absolute paths are stripped from `.out` files.

## Files

- Chapters `F2-43.html` … `F2-50.html`: all 21 template sections plus "Answers to Check
  yourself" with a forensic answer key; jargon box; transition box; at least one inline SVG
  figure (F2-43 has two); claim tags; line-by-line tables for the main listings. All eight
  pass the fragment checks (html.parser balance, ids prefixed with the chapter id, no URLs,
  no `<script>`, section ids in order, every claim tag resolves, every `data-src`/`data-run`
  file exists).
- `glossary.json`: 47 entries. Ten reuse a term that another course already defines, with
  that course's definition copied verbatim so the build's merge (first course alphabetically
  keeps the text) changes nothing: "Section", "Map file", "Linker script" (HW303), "Calling
  convention (ABI)", "Stack frame" (HW202), "Memory-mapped I/O (MMIO)", "Device register",
  "Side effect (of a register access)", "volatile (C++)" (HW204). SP101's "Object file",
  "Symbol", "Linker" and "Name mangling" are linked from the text, not redefined; SP301 uses
  the narrower names "Relocatable object file", "Symbol table", "Symbol binding".
- Labs: `university/labs/F2-43` … `F2-50`.

## Listings and runs (55 recorded steps, plus F2-44's unit tests and one-hour fuzz run)

| result | which |
|---|---|
| built and ran, exit 0 | F2-43 objects, linked, forensic_build, forensic_fixed; F2-44 elf_tests, binaries, compare (6 × MATCH), elfread_sample, header, fuzz_short, forensic_exec; F2-45 build, compare (2 × MATCH), pe_sample, relocs; F2-46 build, higher, forensic_fixed; F2-47 sysv, win64, arm64, redzone, call_asm, vtable, forensic_O0, forensic_fixed; F2-48 p1_clone, p1_inspect, guard; F2-49 asm_demo, no_volatile, forensic_O0, forensic_disasm; F2-50 qemu_pci, codegen |
| booted in QEMU, exit status by design (33 = success via isa-debug-exit, 67 for the UEFI app) | F2-45 boot (67); F2-46 boot (33); F2-48 kernel (33); F2-49 kernel (33); F2-50 kernel (33) |
| expected link failure | F2-43 far ("relocation truncated to fit"); F2-48 missing_runtime (undefined `operator new`, `__cxa_atexit`, …) |
| expected compile failure | F2-48 errors (throw with -fno-exceptions in clang and g++, dynamic_cast with -fno-rtti, `<cstdint>` not found) |
| intentional wrong behaviour (forensic evidence) | F2-43 forensic_build (weak default wins); F2-44 forensic_exec (Exec format error, 126); F2-45 forensic_fixed (firmware refuses /fixed image, shell, time limit 124); F2-46 forensic_new (QEMU refuses kernel, 1); F2-47 forensic_O2 (SIGSEGV 139); F2-48 forensic_old (global not constructed, 35); F2-49 user_cr0 (SIGSEGV 139), forensic_O2 (exit 1); F2-50 forensic_O2 (prints factorial 10), forensic_O0 |
| one-hour run | F2-44 `fuzz_hour.sh` (not called by `run.sh`; run once, log `fuzz_hour.log`) |

No log contains "UNEXPECTED" or "STEP FAILED".

### Untested on hardware

All boots ran in QEMU 8.2.2 (TCG): F2-45 (q35 + OVMF), F2-46, F2-48, F2-49, F2-50 (with
QEMU's `edu` device, which exists only in QEMU). The lab kernels print through QEMU's port
0xe9 debug console and exit through `isa-debug-exit`; on a physical PC they would run silently.
Host programs ran on the container's virtualised processor (F2-49's CPUID values are what the
hypervisor presents). Each chapter has a "Watch out — Untested on hardware" box where relevant
(F2-45, F2-46, F2-48, F2-49, F2-50), and the logs carry a `hardware:` line.

## Per chapter: unverified claims and boxes

Every D-source is "title only — not opened during this build" (dossier gate G1 open). Claims
that rest on the runs cite R-sources. Claims written from memory carry an unverified box:

- **F2-43** (1 box): x86-64 relocation type names, numeric values and formulas (S + A,
  S + A − P, L + A − P) from memory of the AMD64 psABI. Verified only: S + A − P reproduces
  four PC32/PLT32 fields byte for byte. Also from memory without a box: GNU ld archive
  scanning order and `--trace-symbol` (stated as "check in the ld manual").
- **F2-44** (2 boxes): ELF extended numbering (`SHN_XINDEX`, `SHN_LORESERVE`), reserved
  section indices ABS/COMMON; the Linux kernel's and dynamic loader's load sequence. Field
  offsets are verified by `static_assert` plus line-for-line agreement with `readelf` on six
  files. The statement that ELF parsers have had vulnerabilities from trusting header fields
  needs a specific advisory as source before publication (noted in D7).
- **F2-45** (2 boxes): PE field offsets and flag-bit meanings (validated only by agreement
  with `llvm-readobj`); UEFI calling convention, subsystem 10, default path
  `\EFI\BOOT\BOOTX64.EFI`, and relocation by the loader. QEMU's default memory size (128 MiB)
  is stated as recalled. `/Brepro` time-stamp behaviour stated as recalled.
- **F2-46** (1 box): Multiboot header values, 8 KiB search window, alignment; QEMU's PVH
  fallback inferred from its error message.
- **F2-47** (1 box): full caller-/callee-saved register lists for SysV, Win64, AAPCS64; 16-byte
  alignment rule; 128-byte red zone; Windows struct-size rule. Argument registers, stack slots,
  shadow space, hidden result pointers and RBX preservation are observed in disassembly.
- **F2-48** (1 box): C++ rules on dynamic initialisation and freestanding headers; Itanium ABI
  signatures (`__cxa_atexit`, `__dso_handle`, guards); `.ctors` versus `.init_array` history.
  Observation without documentation: LLD did not report `__dso_handle` as undefined.
- **F2-49** (1 box): CR0 and EFLAGS bit meanings, #GP at CPL 3 for CR0 reads and CLI, CPUID
  leaf-0 register order, the `"N"` constraint.
- **F2-50** (2 boxes): PCI configuration-address format, BAR0 offset and low bits, the edu
  register map (all consistent with the run); x86 memory types (MTRR/PAT), Arm device memory
  and barriers, Linux `readl`/`writel` semantics (not observable in TCG).

## Decisions for the owner

1. **P1 toolchain:** no GCC `x86_64-elf` cross compiler was built. The P1 skeleton uses Clang +
   LLD with `--target=x86_64-unknown-none-elf`, which the curriculum's toolchain table allows
   ("GCC cross compiler (x86_64-elf) or Clang + LLD"). Building GCC is offered as an extension.
2. **P1 "second machine":** the lab clones into a second folder of the same container; the
   chapter says so and asks students to repeat on a second computer before counting P1 done.
3. **P2 fuzzing:** no libFuzzer/AFL runtime exists in the container (no `clang_rt` fuzzer
   library), so `elf/fuzz.cpp` is a small mutation fuzzer under ASan/UBSan with a fixed seed,
   not coverage-guided. The chapter states this. The one-hour run is a separate script
   (`fuzz_hour.sh`) so that `run.sh` stays at about two minutes; its log is recorded once.
4. **Boot kernels are 32-bit (i386 Multiboot via QEMU `-kernel`)** for F2-46/48/49/50, while
   P1 targets x86_64. This keeps the labs free of an x86-64 bootloader before track A; the
   chapters note it. The owner may prefer Limine or a UEFI loader later.
5. **F2-45 UEFI app avoids UEFI tables** (prints through QEMU's port 0xe9, exits through
   isa-debug-exit) because UEFI structure layouts could not be verified in this build. A1 will
   use the system table properly.
6. **F2-50 poll count is timing-dependent;** the kernel prints only "at least one" / "none"
   so the expected output stays reproducible. The recorded run printed "none (already done)".
7. **`README.md` files inside `labs/F2-48/p1/`** are lab content (the skeleton repository the
   student copies), not documentation of the build.
8. **Exit codes in logs:** each `.log` records the exit code of the step's *last* command; where
   the interesting status belongs to an earlier command (QEMU exits 33/35/67, crashes 139),
   the R-source text in the chapter says "in the transcript".

## Analogy mapping (proposal for registration)

SP301 uses the F2 world "the restaurant kitchen at work", continuing F2-01's registered
mapping (translator = compiler, translated card with blanks = object file, kitchen manager
Mr Haddad who binds cards = linker). New mappings proposed, one per chapter, to be registered
or replaced by the owner:

| chapter | mapping |
|---|---|
| F2-43 | slip clipped to a card = relocation; "use mine only if nobody else has one" page = weak symbol |
| F2-44 | cooks' table of contents = section headers; van driver's list = program headers; first page = ELF header |
| F2-45 | hotel's binding = PE/COFF; "I would like shelf 140" = ImageBase; list of written page numbers = base relocations |
| F2-46 | shelving plan = linker script; shelf where used vs loading bay = VMA vs LMA |
| F2-47 | trays at the pass = argument registers; knives you must put back = callee-saved registers |
| F2-48 | pop-up stall with no supplier = freestanding; list of ovens to light = .init_array |
| F2-49 | specialist tool with a handling card = extended asm with operands and clobbers |
| F2-50 | order board on the wall changed by helpers = MMIO registers; "always look at the board" = volatile |

Recurring characters (Mr Haddad, Amira, Joon, Kofi, Leila, Tomás, Rafael, Zainab, Lars) are
new names except Mr Haddad (F2-01); Amira is introduced in F2-43. The owner may want to register them.
