# OS201 — Operating-system ideas: author notes

Chapters F3-01 to F3-08, level L2, 4 credits. Prerequisites: SP201 and HW202.
Labs are in `university/labs/F3-01` to `university/labs/F3-08`.

Each lab folder passes `university/labs/run_lab.sh university/labs/<ID>` with status 0. All eight were run again at the end of this build, on 2026-10-09. After that run, the numbers quoted in the prose were checked against the outputs.

## Toolchain and local evidence (as recorded in the logs)

- **Host compiler:** g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0.
  - Course flags: `-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined`.
  - Some runs are deliberately built without sanitizers (`-O0` or `-O2`), because sanitizers change system-call traces, page-fault counts and the address space. Each `run.sh` says why, and its log records the exact command.
- **RISC-V cross compiler:** riscv64-linux-gnu-g++ 13.3.0. It builds the uni-rv kernels with `-ffreestanding -nostdlib -mcmodel=medany -O2`.
- **QEMU:** emulator version 8.2.2 (Debian 1:8.2.2+ds-0ubuntu1.18), using `qemu-system-riscv64 -machine virt -bios none`. Monitor commands are sent over QMP (`qmp_regs.py`, `qmp_cmd.py`).
- **strace:** `strace -- version 6.8`.
- **GNU Binutils 2.42:** objdump, nm and addr2line.
- **Python 3.13.16.**
- **Local source opened:** `/usr/include/x86_64-linux-gnu/asm/unistd_64.h` from linux-libc-dev 6.8.0-146.146 (`__NR_write 1`, `__NR_getpid 39`). It is cited as L1 in F3-01, F3-07 and F3-08.
- **Build machine:**
  - Linux 6.18.44-fc-v80, x86_64, 4 online CPUs, 4096-byte pages, as printed by F3-01's `ask_os`.
  - The file system type was not identified.
  - There are no manual pages and no internet access.
  - **The xv6 source is not in the container.**

## Listings run

**Nothing was run on real hardware.** Every run is one of three kinds: a Linux host program, a C++ model (scheduler, Sv39 walk, toy file system), or the university's uni-rv kernel in QEMU.

| Chapter | Run | Status |
|---|---|---|
| F3-01 | ask_os, hello, ask_name | pass |
| F3-01 | hello_strace, hello_count | pass (strace; 35 calls, 2 errors) |
| F3-01 | frozen | **expected fail by design: exit 124.** `timeout 2` stops a `read` that waits on a pipe. Forensic evidence. |
| F3-01 | frozen_tail | pass (trims `frozen.out`) |
| F3-02 | fork_wait, exec_child, fork_count, zombies | pass |
| F3-02 | noflush | demo, exit 0 (without `fflush`, the parent's buffered line appears after the child's output) |
| F3-03 | threads_share, ctx_switch | pass |
| F3-03 | sched_sim | pass (model) |
| F3-03 | starve | forensic evidence, exit 0 (model) |
| F3-04 | same_address, sv39_walk (model) | pass |
| F3-04 | lazy_pages, lazy_pages_plain, show_maps | pass |
| F3-04 | wild_pointer | **expected fail by design: exit 1.** ASan reports SEGV at 0x20. Forensic evidence. |
| F3-05 | frozen | **expected fail by design: exit 124.** The deadlocked uni-rv machine is stopped by `timeout 6`. |
| F3-05 | cpus | QEMU QMP plus addr2line, exit 0 (both PCs at acquire, `locks.cc:69`) |
| F3-05 | fixed | QEMU, pass (exit 0 through the test device) |
| F3-05 | lockgraph, transfer | pass |
| F3-05 | spin_loop | disassembly, exit 0 |
| F3-06 | file_calls, tinyfs (model) | pass |
| F3-06 | file_strace | pass (strace, trimmed as the log says) |
| F3-06 | crash, crash_fixed | forensic evidence, exit 0 (model) |
| F3-07 | raw_syscall | pass (EFAULT and ENOSYS are intended) |
| F3-07 | raw_strace_file, raw_strace_devnull | pass. The pair shows EFAULT going to a file and 5 going to `/dev/null`. |
| F3-07 | memmap | QEMU `info mtree`, exit 0 |
| F3-07 | syscalls | QEMU, pass |
| F3-07 | device_fault | **expected fail by design: exit 1.** uni-rv kills the process (mcause 7). |
| F3-07 | leak, leak_nm | forensic evidence, exit 0 (`-DBUGGY_CHECK`) |
| F3-07 | user_disasm | disassembly, exit 0 |
| F3-08 | tour, solution_hartid | QEMU, pass |
| F3-08 | hartid_direct | **expected fail by design: exit 1.** Illegal instruction, mcause 2. |
| F3-08 | table_bug | forensic evidence, exit 0 (`-DBUGGY_TABLE`) |
| F3-08 | symbols, sizes | nm and wc, exit 0 |
| F3-08 | xv6 "util" and "syscall" labs (course card) | **untested in this build: no xv6 source** |

The chapters quote two kinds of numbers. Some are stable across reruns: 35 system calls, 50 voluntary switches, 4096 and 4609 faults, PCs, symbol addresses and line counts. Others change with every run, such as raw addresses under ASLR and PIDs, and the prose does not quote them. F3-04 tells the reader to check containment in the output instead.

## Unverified boxes

All D sources are title-only (dossier gate G1 open). The boxes list what was written from memory:

- **F3-01:** glibc caching of `getpid`/`sysconf` results.
- **F3-02:**
  - The meaning of Linux process-state letters other than Z.
  - xv6's process-state names.
- **F3-03:** the Linux scheduler's default policy and time slice for this kernel release.
- **F3-04:**
  - Sv39 details: the bit-38 sign rule, the PTE layout with V/R/W/X/U/G/A/D, megapages and gigapages, `satp`, `sfence.vma`.
  - xv6's separate kernel page table, trampoline and lack of swapping.
  - Linux `vm.mmap_min_addr`.
  - The ASan shadow explanation of 4609 versus 4096 faults. This is labelled a hypothesis in the prose, not a box.
- **F3-05:**
  - xv6 spinlocks disabling interrupts, sleep locks, lock-order rules, sleep/wakeup.
  - Linux lockdep.
  - The `amoswap.w.aq` acquire semantics.
  - The forced interleaving (`step` flags) is disclosed.
  - Untested-on-hardware box: QEMU only, no timing.
- **F3-06:**
  - The xv6 disk layout, root inode number and log.
  - Device write caches, metadata-only journaling defaults, atomicity of `rename`.
  - The crash runs are a model.
  - Untested-on-hardware box: no storage device was tested.
- **F3-07:**
  - RISC-V trap details: MPP bits, PMP NAPOT encoding, the PMP default-deny rule.
  - x86-64 Linux syscall registers.
  - `copy_from_user`, `copyin`, xv6's syscall helpers, OpenSBI.
  - The `/dev/null` interpretation, stated as a hypothesis.
  - Untested-on-hardware box: QEMU memory map only.
- **F3-08:**
  - **Every xv6 file and function name, Figure 1's right column, Figure 2's grey row, and the 6.1810 lab steps.**
  - The CSR privilege encoding.
  - Whether `mtval` holds the instruction bits.
  - Untested-on-hardware box.

## Proposed analogy mappings (F3, the school)

None of these are registered yet. Registered mappings are used as they are: principal and timetable, class and room, student, office window, staff-only rooms, locker map, sticky note, library catalogue, and two students holding books.

| Concept | Proposed analogy | Where it breaks (as written in the chapter) |
|---|---|---|
| CPU core | a teacher who can teach one class at a time (F3-01, F3-03; also used for "hart" in F3-05) | a context switch costs cache and TLB refills |
| Context switch | the teacher leaves a bookmark in the class register, packs up and goes to the next room (F3-03) | bookmarks are free; switches are not |
| fork | the class is split into two identical classes, the new one in a new room with copies of everything (F3-02) | copy-on-write: the copies are made lazily |
| exec | the class keeps its room and register number but swaps every book for a new subject (F3-02) | — |
| Zombie process | the attendance sheet of a class that has gone home, left on the principal's desk until the parent class signs it (F3-02) | — |
| Starvation / aging | a class always moved down the timetable; the longer it waits, the higher it moves (F3-03) | — |
| Page fault | a student asks for a locker number the map does not list; the office assigns a cupboard or reports the student (F3-04) | the "office" is kernel code on the same core |
| Physical frame | one real cupboard (F3-04) | — |
| Demand paging | "no cupboard yet" until the locker is first opened (F3-04) | — |
| Copy-on-write | two classes share a cupboard for reading; the first to put something in gets its own (F3-04) | sharing is deliberate in an OS, an error in a school map |
| Lock | the one key to a shared cupboard (F3-05) | a lock is a convention; code can ignore it |
| Spinlock | standing at the cupboard trying the handle again and again (F3-05) | spinning slows other cores (cache-line traffic) |
| Sleeping lock | writing your name on the list and sitting down until called (F3-05) | — |
| Lock ordering | keys are always collected in alphabetical order (F3-05) | — |
| Inode | the catalogue card of one book: size and shelves (F3-06) | — |
| Directory | a list of titles, each with a card number (F3-06) | the catalogue is itself on the shelves |
| Hard link | two titles pointing to one card (F3-06) | — |
| File descriptor | a borrowing slip that keeps the book from being thrown away (F3-06) | a nameless file lives on while open |
| fsck / journaling | the librarian's shelf check / notebook written before shelving (F3-06) | power loss erases memory, not hands |
| Trap frame | the clerk copies the student's desk onto a tray and puts it back (F3-07) | the same hart runs program and kernel |
| System-call number / errno / EFAULT | the form number / the reason stamped on a refused form (F3-07) | not a polite request (registered) |
| Machine mode / PMP | the principal's keys / a fence around the student area (F3-07) | — |
| Reading a kernel | a first-day tour following one visitor (F3-08) | code paths are paths in time, not space |

## Decisions for the owner

1. **xv6 is not in the build container, and there is no internet.**
   - The course card's labs (6.1810 "util" and "syscall") are therefore untested in this build.
   - F3-08 gives their steps from memory, inside an unverified box, and quotes the P4 acceptance verbatim: "xv6 lab grading scripts pass."
   - Decide whether a later build gets the xv6-riscv source (pinned commit) and the RISC-V toolchain its Makefile expects, so the labs can be run and F3-08's file map checked.
2. **uni-rv stands in for xv6.** uni-rv is the university's own small RISC-V kernel, in machine mode with PMP, under 350 lines. It provides the forensic "frozen machine" (a real two-hart deadlock), the system-call path, and "add a system call". Decide whether uni-rv stays as a permanent teaching kernel, or only as a bridge to xv6.
3. **The course card asks for "an xv6 trace where two processes deadlock on locks".** The forensic lab instead uses a uni-rv trace with the same structure: rename and unlink on two harts, with directory and inode locks. Its bad interleaving is forced by `step` flags so that the run is reproducible, and the chapter says so. Decide whether the xv6 version is still required once xv6 is available.
4. **The answer key `solution_hartid.cc` sits in the lab folder** (`university/labs/F3-08`), so learners can see it. It is marked "ANSWER KEY" at the top. The lab runner needs it there to produce the expected-observation run. Decide whether answer keys should move to an instructor-only location.
5. **New lab run `spin_loop` in F3-05.** It is the objdump of `acquire` and was added late in this build. Its log's "machine" line mentions the emulated machine, although objdump runs on the host. This is harmless, but it is noted here.
6. **Glossary overlap with SP203.** SP203 is being written concurrently and now defines Deadlock, Spinlock, Critical section, Lock ordering, Wait-for graph and Mutex.
   - OS201's glossary does not redefine them, and the F3-05 chapter links to them.
   - If SP203 drops any of these, F3-05's links break. A rebuild would show it.
   - OS201 adds "Lock" and "Sleeping lock" as kernel-side terms.
7. **Analogy registrations.** The table above lists the mappings proposed for the guide's section 8.1 map.
