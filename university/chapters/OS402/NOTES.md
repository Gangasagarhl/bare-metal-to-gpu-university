# OS402 — User space and self-hosting: author notes

Chapters F3-50 to F3-55. Level L4, 5 credits. Prerequisites: OS304 and DR301 (C6). Maps to curriculum section 8, milestones U1–U5. Course project: U5 (your OS compiles and runs a C++ program inside itself).

Labs are in `university/labs/F3-50` to `university/labs/F3-55`. All six pass `university/labs/run_lab.sh university/labs/<ID>` with exit code 0. All six were re-run at the end of this build (2026-10-09), after the last change to any lab file. F3-55 was re-run once more after its forensic step got an explicit expected-fail check.

## Checks every fragment passes

- html.parser balance.
- Every id prefixed with the chapter id.
- No URLs and no `<script>`.
- All 21 template sections plus the answers, in the order of FRAGMENT_FORMAT.md.
- Every in-chapter link resolves.
- Every `data-src` and `data-run` file exists.

`python3 university/build/build.py` runs with exit code 0 and reports **no PROBLEM line for F3-50 to F3-55**. The remaining PROBLEM lines are broken `#gl-…` links of other courses whose glossaries are still being written (for example OS401's F3-43).

**Length** (prose, excluding code, outputs, SVG and tables; including the jargon box, sources and answers):

| Chapter | Words |
|---|---|
| F3-50 | about 6,100 |
| F3-51 | about 5,200 |
| F3-52 | about 6,100 |
| F3-53 | about 5,500 |
| F3-54 | about 5,300 |
| F3-55 | about 5,100 |

**Glossary.** `glossary.json` holds 56 four-part entries, generated from the chapters' jargon boxes, so the wording is identical in both places. Some entries collide with other courses' glossaries:

- "errno" also exists in OS201.
- "Relocation" and "ABI (application binary interface)" also exist in SP301.
- `build.py` merges entries with the same term and keeps the first course's wording. The meanings agree, so this is harmless.
- HW101 has an electrical "Signal", so F3-51's term is entered as **"Signal (POSIX)"** (anchor `gl-signal-posix`).

## Toolchain (as recorded in the logs)

- **Compilers and libraries:**
  - g++ / gcc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0;
  - aarch64-linux-gnu-g++ and riscv64-linux-gnu-g++ 13.3.0;
  - Ubuntu clang 18.1.3 and LLD 18.1.3;
  - glibc 2.39 (ldd 2.39-0ubuntu8.9).
- **Binary tools and emulation:**
  - GNU Binutils 2.42 (readelf, objdump, nm, as, ld);
  - QEMU 8.2.2 user mode (qemu-aarch64, qemu-riscv64), used with `-L /usr/<triple>`.
- **Other tools:**
  - strace 6.8;
  - GNU config.sub 2022-01-03;
  - GNU tar 1.35;
  - cmp/diff 3.10.
- **Headers opened in this build:**
  - the Linux UAPI headers from linux-libc-dev 6.8.0-146.146 (and the cross packages 6.8.0-25.25cross1): `asm/unistd_64.h`, `asm-generic/unistd.h`, `asm-generic/errno*.h`;
  - glibc's `dlfcn.h`.
- **Shared helper.** `university/labs/F3-50/oslib.sh` is sourced by every OS402 `run.sh`. It provides the version strings, `rec` (writes a `.log` with the runner's fields) and `expect` (a step whose exit code differs from the expected one sets the script's status to 1).
- **Generated binaries.** All are built in `.o/` and deleted at the end of each `run.sh`.

## Listings run

Status codes:
- **pass**: exit code 0, or the documented code;
- **expected-fail**: a deliberate failure, documented in the log's `note:` line and checked with `expect`;
- **UoH**: untested on hardware (QEMU user mode only).

The U-milestone runs inside the learner's own OS are untested in this build for every chapter. No learner OS exists here, and each chapter says so in an unverified box.

### F3-50 Choosing a system-call ABI

| Run | Status | What it shows |
|---|---|---|
| raw_syscall | pass | Raw `syscall` and libc wrappers side by side: −9 vs −1/errno 9; −38 vs −1/errno 38. |
| nolibc_build, nolibc_x86 | pass | Freestanding hello on x86-64. |
| nolibc_a64, nolibc_rv | pass, UoH | The same program on AArch64 and RISC-V under QEMU user mode. |
| instr | pass | The trap instructions in the three binaries (`syscall`, `svc #0`, `ecall`). |
| syscalls | pass | 10 / 18 / 36 system calls for the freestanding, static and dynamic hello. |
| enosys_probe | pass | `clone3` forced to ENOSYS; glibc falls back to `clone`. |
| vdso | pass | 0 `clock_gettime` system calls for 1,000 calls (vDSO). |
| compat | pass | A miniature Linux-compatible dispatch layer (Listing 3). |
| forensic_logger | pass | Forensic evidence: a compat layer returning +errno ("short write, 9 of 8 bytes"). |

### F3-51 A conforming C library and POSIX subset

| Run | Status | What it shows |
|---|---|---|
| posix_subset | pass | 12 of 12 behaviour tests PASS; excluded areas listed with reasons. |
| sigint_pty | pass | U1 signal test on a pseudo-terminal: −1/EINTR without SA_RESTART, restarted read with it. |
| forensic_workers, forensic_tty, forensic_compare | pass | stdio buffer duplicated by `fork`: 6 lines to a file, 4 on a terminal. |

### F3-52 Dynamic linking and thread-local storage

| Run | Status | What it shows |
|---|---|---|
| auxv | pass | Auxiliary vector vs `dl_iterate_phdr`: three "matches: yes". Addresses vary per run. |
| chain, chain_elf, chain_bindings | pass | U2 test 1: chain of two libraries; interposition (the executable's `who` wins); `LD_DEBUG=bindings` evidence. |
| tls_dlopen | pass | U2 tests 2 and 3: TLS in 8 threads; 50,000 dlopen cycles with a flat heap after batch 1. |
| tls_code | pass | Local exec (`%fs:-4`) in the program, `__tls_get_addr` in the plugin. |
| forensic_run, forensic_syms, forensic_fixed | pass | Surprise interposition of an internal `validate` (total 3, expected 12); fixed with `-fvisibility=hidden`. |

### F3-53 A hosted C++ runtime

| Run | Status | What it shows |
|---|---|---|
| u3_build, u3_x86 | pass | U3 test program with sanitizers on x86-64. |
| u3_a64, u3_rv | pass, UoH | The same program under QEMU user mode. |
| u3_compare | pass | All three outputs identical (419 bytes). |
| terminate_demo | pass | Uncaught exception: terminate message, child ends by signal 6, parent continues. |
| phdr_walk, phdr_check | pass | `.eh_frame_hdr` FDE counts via `dl_iterate_phdr`; cross-check against readelf (6 = 6). |
| unwinder_lookup | pass | libgcc_s (three architectures) and libgcc_eh import `_dl_find_object`, not `dl_iterate_phdr`. |
| forensic_run | **expected-fail** (exit code 134) | The card's forensic: an exception from a dlopen'ed library ends in `std::terminate`. |
| forensic_fixed | pass | The lookup table follows dlopen: "caught … code 7". |

### F3-54 An OS-specific toolchain and ports

| Run | Status | What it shows |
|---|---|---|
| triple_config | pass | `config.sub` rejects `x86_64-myos`; a patched copy gives `x86_64-pc-myos`. |
| triple_clang | pass | Clang accepts the triple; 375 vs 389 macros; only `__ELF__` remains. |
| driver_trap | pass | Clang's planned link step for myos calls `/usr/bin/gcc`. |
| sysroot, hello_myos | pass | Teaching sysroot and wrapper; canary: `__myos__` defined, `__linux__` not. Runs on the host via the Linux-compatible ABI. |
| repro | pass | Plain tar: both packages differ; deterministic tar: hello identical, banner differs. Hashes vary per run. |
| ports | pass | 10 recipes in 4 waves. |
| ports_broken | **expected-fail** (exit code 1) | A missing dependency and a cycle are reported. |
| forensic_repro, forensic_fixed | pass | `__DATE__`/`__TIME__` embedded; `SOURCE_DATE_EPOCH` makes both packages identical. Byte counts and offsets vary per run. |

### F3-55 Self-hosting

| Run | Status | What it shows |
|---|---|---|
| minicc | pass | The tiny compiler's 108-line assembly output. |
| stage_compare | pass | A = A2 (deterministic), A ≠ B (g++ vs clang++), identical outputs; the program prints 42, −13, 16, −4. |
| compile_trace | pass | One g++ compile: g++, cc1plus, as; about 8,600 system calls of 33 kinds; openat 380 succeeded, 662 failed. Counts vary slightly. |
| triples | pass | `--build`/`--host`/`--target` of three installed GCCs. |
| measure | pass | Calibration shows ru_maxrss in KiB; the compile takes about 2 s with a peak of about 185 MiB. Varies per run. |
| forensic_compile | **expected-fail** (exit code 1) | The `as` error under `strace -e inject=lseek:error=ENOSYS`. |
| forensic_trace | pass | Failed calls per program; the only ENOSYS is 2× `lseek` in `as`. |

## Unverified boxes (what an owner or Source Researcher must check)

No official document could be opened in this build. Every D-tag is "title only — pending verification (dossier gate G1 open)". The boxes below name the specific claims that most need checking.

### F3-50

- The exact range of negative return values Linux C libraries treat as errors (−4095..−1 as commonly stated).
- The AArch64 and RISC-V runs used QEMU user mode, not hardware (UoH).

### F3-51

- Which Linux system calls restart after an `SA_RESTART` handler and which always fail with EINTR.
- The organisation, build and result format of libc-test and the Open POSIX Test Suite.
- U1 inside the learner's OS: untested.

### F3-52

- PIE and interpreter placement rules, and the auxiliary-vector entries beyond the five that U2 names.
- Relocation type names and numbers. They are deliberately not given; take them from the psABIs or `elf.h`.
- `arch_prctl(ARCH_SET_FS)`, `CLONE_SETTLS` and user-mode `WRFSBASE`.
- TLS sequences and the variant I layout on AArch64 and RISC-V were not compiled here (UoH).
- U2 inside the learner's OS, and the musl/mlibc dynamic linkers: untested.

### F3-53

- **The `.eh_frame_hdr` layout and encoding values decoded by `phdr_walk.cpp` were written from memory.** They are cross-checked against readelf for one program only (6 = 6).
- Which lookup interface LLVM libunwind uses, and libgcc's fallback before glibc 2.35.
- C++ runtime build options for a new triple (none given).
- QEMU-only runs (UoH).
- U3 inside the learner's OS: untested.

### F3-54

- Why Clang delegates linking to `gcc` for an unknown OS. This is inferred from `-###` output.
- GCC/Binutils target files and options: no compiler was patched.
- The recipe formats of SerenityOS Ports, xbstrap and pkgsrc, and the CMake/Meson cross-file syntax.
- No program ran inside a myos kernel (UoH).
- U4 (second machine, ported programs' test suites): untested.

### F3-55

- GCC bootstrap details and LLVM multi-stage builds. No bootstrap was run.
- Two inferences from the trace: why cc1plus calls `readlink` so often, and why GCC calls `sysinfo`.
- The forensic answer key's statement that BFD seeks before each section write. It is marked in the text as inferred from the trace.
- U5 in QEMU or on the spare PC: untested (UoH).

### Smaller inference, marked in the text

- F3-53 answer 5: why the sanitizer build has more FDEs (9 vs 6). This was not checked.

## Analogy extensions (proposals, not used as registered mappings)

The registered F3 school mappings used in these chapters:
- system call = asking at the office window with a form;
- user/kernel mode = student areas and staff rooms;
- process = a class with its own room;
- the OS = the principal.

The chapters' analogy boxes use the extensions below. Each is stated as an analogy inside its chapter and is **proposed here for registration**:

| Chapter | Proposed mapping |
|---|---|
| F3-50 | The office's **form catalogue** (form numbers, boxes, reply slips) = the system-call ABI; a "this form does not exist" slip = ENOSYS. |
| F3-51 | The **regional rulebook** = POSIX and ISO C; the **school handbook** = the C library; the handbook page "which window, which form" = the system-dependent layer; the **inspector's checklist** = the test suites; the **fire bell** = a signal; "the librarian stops" = EINTR. |
| F3-52 | **Visiting teachers** = shared objects; the **secretary who settles them in** = the dynamic linker; "report to the secretary" on the badge = PT_INTERP; the welcome note = the auxiliary vector; **the school's own choir leader wins** = interposition; "staff only" = hidden visibility; each pupil's **pencil case** = a TLS block; the **name tag** = the thread pointer. |
| F3-53 | **Evacuation plans in every room** = call-frame information; the floor's list of plans = `.eh_frame_hdr`; the **office list of rooms with plans** = the dynamic linker's object lookup; a teacher waiting at a door = a handler; "check the assembly point first" = two-phase unwinding; the new wing missing from the list = the forensic fault. |
| F3-54 | **The school's own textbook edition** = an OS-specific toolchain; the school name on the printer's form = the target triple; the school's own pictures = the sysroot; the **print room's order list** = the ports tree; "print after" = a dependency; the edition date = SOURCE_DATE_EPOCH. |
| F3-55 | **A school that trains its own teachers** = self-hosting; the outside college = the cross toolchain (Canadian cross); the **word-for-word lesson comparison** = the stage comparison. |

## Decisions for the owner

1. **The forensic of the course card versus the host's unwinder (F3-53).**
   - The card says "find the missing dl_iterate_phdr support". On this host, libgcc's unwinders (all three architectures, shared and static) call glibc's `_dl_find_object` instead (R3 of F3-53).
   - The lab therefore reproduces the defect by interposing `_dl_find_object` with a port-style table that does not follow `dlopen`. The answer key states that the diagnosis is the same for a `dl_iterate_phdr`-based unwinder.
   - Options: accept this, or reword the card ("…find the missing object-lookup support (dl_iterate_phdr or _dl_find_object)").
2. **Each other chapter has its own forensic case.** The card names only one forensic, and it is used in F3-53. The other cases were chosen to fit each chapter:
   - F3-50: a compat layer returning +errno;
   - F3-51: duplicated log lines after fork;
   - F3-52: surprise interposition;
   - F3-54: a non-reproducible package from `__DATE__`/`__TIME__`;
   - F3-55: an assembler failure caused by a missing `lseek`.
3. **The teaching "myos" toolchain uses the Linux-compatible ABI (F3-54).** The sysroot's C library uses Linux system-call numbers (F3-50 path 2), so `hello_myos` can be run in the build container. The chapter says so. If the owner prefers the course to model path 1 (own ABI), the run would become "build only, untested".
4. **A wrapper instead of a patched compiler (F3-54).** No GCC, Binutils or LLVM was patched (time and scope). The wrapper plus `ld.lld` shows the sysroot discipline and avoids the observed driver trap. Patching is left to the milestone with unverified boxes.
5. **The stage comparison in miniature (F3-55).** `minicc` cannot compile itself. The lab compares the outputs of two differently built compilers instead of compilers built by compilers, and the text says so explicitly.
6. **"Memory statistics returning to baseline" (F3-52).** A naive before/after test fails on glibc without any leak: the heap grows once during the first cycles. The listing defines the baseline as "no growth over the last three batches of 10,000". The owner may want this definition written into the U2 acceptance wording.
7. **Source registry.** F3-54 cites "Reproducible-builds documentation / SOURCE_DATE_EPOCH specification" (D5), which is not in the curriculum's reading list. It is proposed for the registry.
8. **Exam P.** The card's P exam ("rebuild a ported program inside your OS") has no separate material beyond F3-54's ports and F3-55's lab steps. The owner may want a dedicated exam brief.

## Style notes

- About 130 lines in the OS402 lab sources exceed 100 characters. Most are `printf` format strings, `rec` lines in `run.sh` and long command lines. They compile and run cleanly. Wrapping them is low priority, but would help the rendered listings on narrow screens.
- Some outputs change on every run, and the prose describes them generically, quoting only stable verdicts:
  - addresses in `auxv.out`;
  - hashes and byte offsets in F3-54;
  - system-call counts, times and memory in F3-55.
