# OS303 — Kernel II: threads, SMP, user mode and processes: author notes

Chapters F3-26 to F3-30. Level L3–L4. Curriculum Track B, milestones B10–B13 (threads and scheduling, synchronisation, SMP bring-up, user mode and system calls, processes and ELF loading).
Labs are in `university/labs/F3-26` to `university/labs/F3-30`.

Each lab passes `university/labs/run_lab.sh university/labs/<ID>` with exit code 0. All five were re-run in this build on 2026-10-09, and the numbers quoted in the prose were checked against the outputs of the last run.

Every fragment passes the local checks:
- html.parser balance;
- ids prefixed with the chapter id;
- no URLs;
- no `<script>`;
- section order (21 sections plus Answers);
- source references resolve;
- every `data-src` and `data-run` file exists.

`python3 university/build/build.py` reports no PROBLEM line for F3-26 to F3-30 or for the OS303 glossary. The PROBLEM lines it still prints belong to other courses' chapters that were being written at the same time.

## One teaching kernel, five folders

All five chapters build one kernel, `kernel.elf`, from the sources in all five lab folders. The build is `F3-26/build_kernel.sh`; the shared helpers (`build_step`, `qemu_step`, `rec`) are in `F3-26/labtools.sh`.
- Each test is selected on the kernel command line (`test=threads`, `test=sync`, `test=smp`, `test=user`, `test=proc`, ...).
- An exit code of 1 through `isa-debug-exit` means the test passed. Exit code 3 means the kernel reported a failure or panicked. The forensic runs expect exit code 3.
- Forensic variants are built with `-D` switches: `BUG_TRUST_USER_POINTER` and `BUG_LEAK_PAGE_TABLES`. The F3-26 and F3-27 forensic cases use command-line switches instead.
- User programs (`u_*.elf`) are linked at 0x80_0040_0000 and passed to QEMU as Multiboot modules (`-initrd`).

## Toolchain and local evidence (as recorded in the logs)

- **g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0 and GNU ld 2.42.**
  - The kernel is built freestanding with `-std=c++20 -O2 -g -ffreestanding -fno-exceptions -fno-rtti -fno-stack-protector -fno-pic -fno-pie -mno-red-zone -mgeneral-regs-only -fno-omit-frame-pointer` (see `build_kernel.sh`).
  - User programs add `-static -nostdlib -mcmodel=large`.
  - QEMU's Multiboot loader needs a 32-bit ELF, so `kernel.elf` is converted to `kernel32.elf` with `objcopy -O elf32-i386`.
- **Host models**, built with g++ 13.3.0 and `-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined`:
  - `rr_model`, `prodcons`, `lockorder`, `uaccess_test`, `elf_fuzz`.
- **QEMU emulator version 8.2.2.**
  - Machine `-M pc` with TCG. No KVM was available in the container.
  - `-cpu max` is used for SMEP and SMAP in F3-29 and F3-30.
  - `-smp 1` to `-smp 16`.
- **GNU readelf 2.42 and addr2line 2.42.** Used to resolve the forensic `rip=` addresses to source lines.

## Listings run

**Nothing was run on real hardware.** Every QEMU run is untested on hardware.

| Chapter | Run | Status |
|---|---|---|
| F3-26 | build | pass (one kernel from all five folders) |
| F3-26 | rr_model (host) | pass: cooperative, nothing finishes; round robin with quantum 2, editor finishes at slice 8 and backup at 16 |
| F3-26 | threads | pass (exit 1): 100,000 create/exit cycles with frames, heap and stack slots back at the baseline. Untested on hardware |
| F3-26 | forensic_overflow | **expected fail (exit 3)**: kernel-stack overflow into the guard page, then a double fault on IST1 and a panic |
| F3-26 | forensic_addr2line | forensic evidence, exit 0 |
| F3-27 | build | pass |
| F3-27 | prodcons (host) | pass: 10,000,000 items, 4 producers and 4 consumers, missing 0, duplicated 0 |
| F3-27 | lockorder (host) | pass: 1 inversion reported, 0 with ordered locking |
| F3-27 | sync | pass (exit 1): 10M items through the kernel bounded buffer; semaphore; lockdep A/B; mutex in an IRQ reported; irq-shared counter correct. Untested on hardware |
| F3-27 | forensic_irqlock | **expected fail (exit 3)**: lockup detector reports cpu0 spinning on 'stats' it already holds, from an interrupt handler |
| F3-27 | forensic_addr2line | forensic evidence, exit 0 |
| F3-28 | build | pass |
| F3-28 | madt | pass (exit 1): 8 processors, an I/O APIC and 5 overrides in a 176-byte MADT |
| F3-28 | cpus_1, cpus_2, cpus_4, cpus_8, cpus_16 | pass (exit 1): every AP started. Untested on hardware |
| F3-28 | smp8 | pass (exit 1): 200k items; 20 short runs; TLB shootdown, all 8 CPUs faulted, 0 stale reads. Untested on hardware |
| F3-28 | noshootdown | **expected fail (exit 3)**: with shootdown disabled, one CPU keeps reading through a stale TLB entry |
| F3-28 | abba_1cpu | pass (exit 1): on one CPU the ABBA order is reported by lockdep but does not hang |
| F3-28 | abba_8cpu | **expected fail (exit 3)**: ABBA deadlock between two CPUs, found by the lockup detector |
| F3-28 | forensic_addr2line | forensic evidence, exit 0 |
| F3-29 | build, readelf_hello | pass |
| F3-29 | uaccess_test (host) | pass: `user_range_ok` wrong in 0 of 9 cases, the naive check wrong in 2 |
| F3-29 | user | pass (exit 1): SYSCALL/SYSRET, SMEP and SMAP on; bad pointers rejected with −2; ring-3 faults kill only the process. Untested on hardware |
| F3-29 | build_bug | pass (with `-DBUG_TRUST_USER_POINTER`) |
| F3-29 | forensic_smap | **expected fail (exit 3)**: SMAP catches the kernel reading a user pointer directly |
| F3-29 | forensic_nosmap | **expected fail (exit 3)**: with `nosmap=1` the same bug reads an unmapped user address; addr2line traces it to `sys_write` |
| F3-29 | forensic_addr2line | forensic evidence, exit 0 |
| F3-30 | build, readelf_child | pass |
| F3-30 | elf_fuzz (host) | pass: 200,000 mutants under ASan and UBSan; 0 accepted files that break a loader promise |
| F3-30 | proc | pass (exit 1): argv and auxv; init reaps ten children with statuses 10 to 19; kill gives 0x1FF; memory at baseline. Untested on hardware |
| F3-30 | execloop | pass (exit 1): 10,000 exec/exit cycles, free frames constant at the baseline. Untested on hardware |
| F3-30 | fuzz | pass (exit 1): 3000 mutants through the kernel exec path; 1877 rejected for 16 reasons; 1123 ran; 0 kernel faults. Untested on hardware |
| F3-30 | build_bug | pass (with `-DBUG_LEAK_PAGE_TABLES`) |
| F3-30 | forensic_leak | **expected fail (exit 3)**: 5 frames leak per cycle (64,749 down to 14,749 free frames) |

## Unverified boxes (by chapter)

All book and specification sources are cited by title only. None was opened during this build (dossier gate G1 is open). Each chapter has one "Not verified" box:
- **F3-26.** Interrupt-frame layout and the IST mechanism. The double-fault rules (#PF while delivering #PF). The TSS layout. The psABI callee-saved register set. The local-APIC rule that only one interrupt per vector is pending.
- **F3-27.** The x86 memory-ordering guarantees the ticket lock relies on (`lock xadd`, TSO). PAUSE semantics. The interrupt flag and `pushfq`/`popfq` behaviour.
- **F3-28.** The INIT-SIPI-SIPI sequence and delays. The SIPI vector rule (page number). The local-APIC ICR layout. The MADT entry formats and the RSDP/RSDT layout. That a CR3 load or INVLPG flushes only the local TLB.
- **F3-29.** The SYSCALL/SYSRET MSRs (STAR, LSTAR, FMASK) and the selector arithmetic. The SYSRET non-canonical-RCX issue. The SMEP and SMAP bits in CR4 and the STAC/CLAC rules. The EFLAGS.AC behaviour.
- **F3-30.** ELF64 header and program-header offsets and constants. The AT_* numbers. The initial stack layout and alignment. Page-fault restart semantics. Partial independent evidence: readelf output and `u_args` reading its own AT_ENTRY.

## Deviations from the curriculum's wording (stated in the chapters)

- **F3-28.**
  - The SMP stress tests are reduced: 200k items plus 20 runs of 10k, instead of 10M items × 100 runs. An earlier 10M run on 8 CPUs under TCG did not finish in 5 minutes.
  - TCG only; no KVM.
- **F3-29.** The acceptance test's "misaligned length" is read as lengths that run past the mapped range and lengths that wrap around 2^64.
- **F3-30.**
  - Processes have one thread each; B13 names "processes with multiple threads".
  - The "initial RAM disk" is QEMU's Multiboot modules.
  - The "1 hour of fuzzing" is reduced to 3000 kernel mutants plus 200,000 host mutants of blind (not coverage-guided) mutation.
  - Processes are created with `spawn`, not with `fork` + `exec`. Fork is discussed as a topic.

## Glossary

`glossary.json` holds the 36 new terms. Each term is copied from the jargon box of the chapter that introduces it.

Terms that already exist in other courses' glossaries are linked, not redefined. Examples: Context switch, Spinlock, Mutex, Semaphore, System call, Process, ELF, Demand paging, Zombie process, Fuzzing.

## New analogy mappings proposed (school, F3)

These are for the owner to approve into the analogy register:
- **Teacher.** The CPU.
- **Bell.** The timer interrupt.
- **Notebook.** The kernel stack.
- **Bookmark.** The saved registers.
- **Numbered tickets.** The ticket lock.
- **Hall passes.** The semaphore.
- **Library call list.** The condition variable or wait queue.
- **Desk drawer.** Per-CPU data.
- **Phoning other teachers.** The IPI.
- **Side door 8.** The AP trampoline at 0x8000.
- **Office window, clerk and bag.** The system call, the kernel entry and SMAP.
- **Class with its own room, room plan, caretaker.** The process, the ELF file and the loader.
- **First-day folder.** The initial stack and the auxiliary vector.
- **Head of year.** The init process.

## Decisions for the owner

1. **Kernel placement.** The OS303 kernel is a standalone, low, identity-mapped kernel. It is loaded at 1 MiB with 0–4 GiB identity-mapped, and the user half is PML4[1]. OS302's higher-half kernel was not reused, to keep the labs self-contained. Should OS303 be rebased onto OS302's kernel?
2. **One kernel from five folders.** Later chapters' sources are compiled into the F3-26 build. This keeps one binary, but F3-26's lab depends on files in F3-27 to F3-30.
3. **Reduced SMP sizes and TCG only.** See the deviations above. The tests should be re-run with KVM on a host that has it.
4. **"Misaligned length" interpretation** (F3-29).
5. **Fuzz duration** (F3-30). An hour of coverage-guided fuzzing was not done. The course project asks students to measure it.
6. **Single-thread processes** (F3-30). Multi-threaded processes would need per-process TLB shootdown at exit; the chapter explains this.
7. **Scheduling priorities** are left to the F3-26 mini-project. The scheduler is plain round robin.
8. **No FPU/SSE state** is saved at a switch. All kernel and user code is built with `-mgeneral-regs-only`. Chapter text warns about this.
9. **Outputs vary per run.** Under SMP these vary: CPU masks, which CPU pair deadlocks, tick counts and read counts. The prose numbers were refreshed against the last run and say that they vary; they must be refreshed whenever the labs are re-run.
10. **QEMU Multiboot needs a 32-bit ELF**, hence the objcopy step. This is a QEMU constraint, not part of the kernel design.
