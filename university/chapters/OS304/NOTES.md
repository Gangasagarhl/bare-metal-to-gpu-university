# OS304 — Kernel III: files, IPC and userland: author notes

Chapters F3-31 to F3-35. Level L4, 5 credits. Prerequisite: OS303. Maps to curriculum milestones B14–B18.
Labs are in `university/labs/F3-31` to `university/labs/F3-35`.

Each lab passes `university/labs/run_lab.sh university/labs/<ID>` with exit code 0. All five were re-run at the end of this build, on 2026-10-09, after the last change to any lab file. The numbers quoted in the prose were then checked against the outputs.

Some numbers change from run to run, and the prose says so where they appear:
- the shared-memory handoff time (F3-34);
- the byte count in the watchdog line (F3-34 forensic);
- the pipe inode numbers (F3-35 forensic);
- the mkfs timestamps in the FAT and ext2 listings (F3-32, F3-33).

No prose figure depends on any of these.

Every fragment passes these local checks:
- html.parser balance;
- ids prefixed with the chapter id;
- no URLs;
- no `<script>`;
- no words from the style guide's avoid list;
- every in-chapter `#F3-3x-…` link resolves;
- every `data-src` and `data-run` file exists.

`python3 university/build/build.py` reports no PROBLEM line for F3-31 to F3-35. Before `glossary.json` existed, it reported the glossary links; these now resolve.

Length: about 6,700–7,400 words per chapter by a rough count. The count excludes code, outputs, SVG and tables, but includes the jargon box, sources and answers. The prose proper is near the L4 target of about 5,000 words.

## Toolchain (as recorded in the logs)

- **g++ / gcc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0.**
  - Host programs use the runner's flags: `-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined`.
  - The mini kernel uses `-m32 -ffreestanding -fno-exceptions -fno-rtti -fno-threadsafe-statics -fno-stack-protector -fno-pic -fno-builtin -mgeneral-regs-only -O2 -Werror …`.
- **GNU ld 2.42.** Links the kernel with `ld -m elf_i386 -T kernel.ld`.
- **aarch64-linux-gnu-gcc/g++ 13.3.0.** Builds tlibc for AArch64. **qemu-aarch64 8.2.2** runs it in user-mode emulation.
- **QEMU emulator version 8.2.2 (TCG, no KVM).** Uses `-machine pc -m 64M -nodefaults -display none -no-reboot -monitor none -device isa-debug-exit,iobase=0xf4,iosize=0x04 -serial stdio -kernel … -initrd …`.
- **File-system and system tools:**
  - GNU tar 1.35;
  - mkfs.fat 4.2 and fsck.fat 4.2 (2021-01-31);
  - mtools 4.0.43;
  - e2fsprogs 1.47.0 (mke2fs, e2fsck, debugfs, dumpe2fs);
  - od / coreutils 9.4, diff / cmp 3.10;
  - strace 6.8, procps-ng 4.0.4;
  - Python 3.13;
  - `/bin/sh` = dash 0.5.12.
- **Shared helper.** `university/labs/F3-31/lablib.sh` is sourced by every OS304 `run.sh`. It provides the `rec` log writer, `kbuild`, `hostbuild` and the QEMU base command.
- **Output scrubbing.** The lab scripts remove absolute paths from outputs. The F3-35 forensic step replaces process ids with roles.

## Listings run

Status codes:
- **pass**: exit 0, or the documented pass code;
- **expected-fail**: a deliberate failure, documented in the log's `note:` line;
- **UoH**: untested on hardware.

### F3-31 (B14)

| Run | Status | What it shows |
|---|---|---|
| `build` | pass | Kernel sections: text 7355, rodata 1180, bss 73380. |
| `initrd` | pass | GNU tar; archive of 20480 bytes. |
| `host_view` | pass | The host's view of the archive. |
| `tarhdr` | pass | Raw header dump by an independent Python reader. |
| `boot` | pass (exit 33 = isa-debug-exit 0x10), UoH | — |
| `compare` | pass | Listing MATCH, contents MATCH. |
| `bcache` | pass | 100,000 operations; write-back and write-through, uniform and 80/20. |
| `forensic_bcache` | expected-fail, exit 1 | Dirty bit set only on a miss. |

### F3-32 (B15)

All of these are pass: `lfn`, `build`, `mkfs`, `read`, `write`, `hostcheck` (mtools), `dirdump`, `fsck1`, `stress` (10,000 operations, seed 304) and `fsck2` (fsck.fat clean).

Forensic runs, all exit 0, showing the evidence of a changed checksum line: `forensic_driver`, `forensic_mdir`, `forensic_fsck` and `forensic_dump`. With `-n`, fsck.fat exits 0 even when it reports the checksum errors; the source entry says so.

### F3-33 (B16)

All of these are pass: `build`, `mkfs`, `read`, `write`, `hostcheck` (debugfs), `fsck1`, `stress` (10,000 operations, seed 304) and `fsck2` (e2fsck clean).

| Run | Status | What it shows |
|---|---|---|
| `forensic_run` | pass | Harness with the reordered driver, power cut after 26 writes. |
| `forensic_fsck` | expected-fail, exit 4 | Multiply-claimed blocks. |
| `forensic_stat` | pass | — |
| `fixed_run` | pass | — |
| `fixed_fsck` | expected-fail, exit 4 | Leaks only. |

### F3-34 (B17)

| Run | Status | What it shows |
|---|---|---|
| `pipe_model` | pass | — |
| `pipeline` | pass, on the host Linux kernel | 1 GiB, checksums equal. |
| `shm_event` | pass, on the host Linux kernel | 1,000,000 handoffs, 0 lost. The time is a container measurement. |
| `waitany` | pass | — |
| `forensic_diff` | pass | — |
| `forensic_run` | expected-fail, exit 3 (watchdog) | — |
| `forensic_isolate` | pass | Change 1 alone hangs (3/3); changes 2 and 3 alone pass (3/3). |
| `q8_notify_one` | pass | 3/3 runs passed; used in the Check-yourself answer 8. |

### F3-35 (B18)

| Run | Status | What it shows |
|---|---|---|
| `parse_test` | pass | — |
| `build` | pass | — |
| `shell_session` | pass | — |
| `shell_compare` | pass | Standard output MATCH with dash; standard error differs in wording, reported only. |
| `tlibc_build` | pass | — |
| `ctest_glibc` | pass | — |
| `ctest_tlibc` | pass | — |
| `ctest_a64` | pass | Under qemu-aarch64; untested on Arm hardware. |
| `ctest_compare` | pass | — |
| `syscalls` | pass | — |
| `kbuild` | pass | — |
| `kshell_ref` | pass | — |
| `kshell` | pass, exit 33, UoH | — |
| `kshell_compare` | pass | MATCH, 25 lines. |
| `kshell_naive` | exit 33 | Kept as evidence: the first input character is lost. |
| `forensic_procs` | pass | — |
| `forensic_end` | expected-fail, exit 124 | Time limit. |

### Acceptance status of B14–B18 in this build (honest reading)

**B14.**
- Test 1 ran in kernel mode in the 32-bit mini kernel, not from user programs. The mini kernel has no user mode.
- Test 2 ran as host C++ against a RAM-disk model.
- The VFS of the mini kernel has no `write`, although the B14 build list names `open, read, write, seek, stat, readdir, close`. The chapter's lab steps leave writing to F3-32/33 and the learner's kernel.

**B15.**
- Both tests ran with the host-side driver against image files.
- "With Windows" was not tested; there is no Windows system here. The chapter says so.

**B16.**
- Both tests ran with the host-side driver against image files made by mke2fs 1.47.0.

**B17.**
- Both tests ran as host processes on the container's Linux kernel, as the reference behaviour.
- The kernel objects ran as host-thread models.

**B18.**
- Test 1 ran in two substitute forms: the kernel-mode `kshell` over the serial line against an independent reference, and `minish` against dash on the host.
- Test 2 ran with Linux on x86-64 and on AArch64 (user-mode QEMU) standing in for "your kernel".
- None of these ran as user programs on the course kernel.

## Unverified boxes (per chapter: what to check, and where)

### F3-31

1. **USTAR format details.** Field widths and offsets, NUL termination when fields are full, typeflag values, and how GNU's extensions differ.
   - Check: POSIX `pax` "ustar Interchange Format"; GNU tar manual (format chapter).
2. **Windows cache manager.** The claim that it caches file views rather than disk blocks.
   - Check: Windows Internals 7th ed., Part 2, caching chapter.
3. **16550 UART registers used in `kbase.cc`.** Divisor latch, LCR 0x03, FCR 0xC7, LSR bit 5 and bit 0.
   - Check: the PC16550D datasheet.
4. **Archive blocking.** The 10240-byte record (20 blocks) as GNU tar's default blocking, and POSIX record-size wording.
   - Check: GNU tar manual (blocking); POSIX `pax`.
5. **Untested on hardware.** Booting needs a Multiboot v1 loader with modules (GRUB syntax to be checked) and a serial port. `isa-debug-exit` exists only in QEMU.

### F3-32

1. **FAT type thresholds.** 4085 and 65525 clusters, and the rule that the type is decided only by the cluster count.
   - Check: Microsoft FAT32 specification (FAT type determination).
2. **Case flags.** The lower-case flag bits in byte 12 of the short entry, as used by mtools and Windows, and the full short-name generation rules (allowed characters, tails beyond ~9).
   - Check: Microsoft FAT32 specification; mtools documentation or source.
3. **Write behaviour of real devices.** Sector-write atomicity, 4096-byte physical sectors, and flush and FUA commands.
   - Check: disk datasheet; ACS and NVMe base specification. USB stick power-loss behaviour was not tested.
4. **Untested on hardware.** The driver ran only on image files. Partitioned media need the partition table read first.

### F3-33

1. **ext2 field offsets and reserved inodes.** These are confirmed only by agreement with e2fsprogs. Unused fields were not checked.
   - Check: Poirier, "The Second Extended File System: Internal Layout"; Card, Ts'o and Tweedie; Carrier.
2. **The whole NTFS reading section.** MFT, system files, resident data, run lists, B+ tree directories and the log file.
   - Check: Windows Internals 7th ed. Part 2 (NTFS); Carrier (NTFS chapters); Linux-NTFS documentation.
3. **Device behaviour and defaults.** Torn multi-sector writes and the device write cache, and "4 KiB usual block size".
   - Check: `/etc/mke2fs.conf` and the `mke2fs` manual page.
4. **Untested on hardware.** The power cut is simulated at block granularity only.

### F3-34

1. **`PIPE_BUF` and pipe capacity.** The `PIPE_BUF` atomicity rule and its minimum, and the Linux value and pipe capacity. These are not relied on, and were not measured.
   - Check: POSIX `write`, `<limits.h>`; `pipe(7)`.
2. **Windows and Linux synchronisation objects.** Windows events, `WaitForMultipleObjects` (and its handle limit), ALPC; Linux `eventfd` and futex.
   - Check: Windows Internals 7th ed. Part 1 and Part 2; `eventfd(2)`, `futex(2)`.
3. **Memory ordering.** x86 and ARM memory-ordering statements.
   - Check: Intel SDM Vol. 3A (memory ordering); Arm ARM memory model. The handoff time is a container measurement.
4. **Untested on hardware, and not run on our kernel.**

### F3-35

1. **Porting layers.** mlibc, newlib and musl porting layers ("sysdeps").
   - Check: each library's porting guide.
2. **System-call ABI details.** Register conventions, system-call numbers and the −4095..−1 error range.
   - Check: Linux system-call tables (x86-64, arm64), psABI, AAPCS64, `syscall(2)`.
3. **Lost first serial character.** Hypothesis: `serial_init` writes FCR 0xC7, which clears the receive FIFO and drops input that arrived early. Not tested.
   - Check: move or change the FIFO initialisation and rerun the naive session; PC16550D datasheet.
4. **Untested on hardware.** No serial cable session was run. The AArch64 build ran only under user-mode QEMU. Also unchecked: the `syscall` and `svc` details in Intel SDM Vol. 2 and 3 and the Arm ARM.
5. **Untested on hardware, and not run as user programs on our kernel.** This covers the whole lab.

### Other claims not verified (no box; conservative, tagged to a title-only source)

**Background concepts.** These are tagged D1–D3 in each chapter, and are pending gate G1:
- VFS objects and Linux dentry and page caches (F3-31);
- Bach's buffer cache (F3-31);
- next-fit allocation and fragmentation (F3-32);
- FFS cylinder groups and ext3 ordered mode (F3-33);
- soft-updates rules (F3-33, the Ganger et al. paper is named);
- the Windows dispatcher, wait-block design and signal semantics (F3-34);
- the System V initial stack layout (F3-35).

**Offsets in code.** Multiboot v1 magic and flags, FAT BPB and LFN offsets, and ext2 superblock offsets are confirmed only by interoperation in the runs (QEMU, mtools, fsck.fat, e2fsprogs). The Sources lists say so.

## Analogy proposals (F3 school world). Owner approval needed.

The registered mappings are used as they are: the file system as the library catalogue and shelves, the process as a class with its own room, the system call as the office window, and deadlock as two students each holding a book. Proposed new mappings:

| Concept | Proposed school mapping | Chapter | Where it breaks (stated in the chapter) |
|---|---|---|---|
| VFS | the front desk that routes request slips to the right library | F3-31 | A mounted file system does not know its mount point. |
| Block cache | the librarian's trolley of recently used books | F3-31 | The cache can hold the only correct copy. |
| Initial RAM disk | the box of books that arrives with the principal on day one | F3-31 | The archive is served in place, not unpacked. |
| FAT / cluster chain | the ledger of "continued on shelf n" lines | F3-32 | Two FAT copies can disagree; clusters have no owner field. |
| LFN entries | extra title cards with a check number in front of the old card | F3-32 | The tie is only a checksum. |
| Block group / bitmap / inode | a floor with its free-shelf chart and drawer of record cards | F3-33 | The bitmap is a second record of the same fact. |
| Pipe | the message tube between classrooms | F3-34 | Ends are shared after fork. |
| Shared memory | the whiteboard in the wall between two rooms | F3-34 | Memory ordering. |
| Event | the bell | F3-34 | A wake-up goes only where the code sends it. |
| Shell | the receptionist who takes spoken requests and hands work to helpers | F3-35 | The shell copies itself (fork). |
| C library / system-call layer | the school's standard forms, and the office-specific version of the forms | F3-35 | The ABI below the names must match too. |

Analogy share: stories and analogies stay inside the hook, Layer 1 and the jargon "Analogy" parts. This is estimated at well under 10% of the prose, but it was not measured precisely.

## Decisions for the owner

1. **A separate 32-bit Multiboot mini kernel for the labs (F3-31, reused in F3-35).** The learner's OS302/OS303 kernel is x86-64 with paging and user mode. To keep every line about files, OS304's in-kernel labs use a small 32-bit kernel with no paging and no user mode, booted by QEMU's built-in Multiboot v1 loader with the initrd as a module. The VFS, tarfs and shell code are written to move into the learner's kernel.
   - Decide whether to keep this, or to require labs to build on a reference OS303 kernel (none exists in the repository yet).
2. **Host-side drivers and models.** FAT32 and ext2 run as host C++ over image files, with the host tools as judges. The pipe and wait-for-multiple objects run as host-thread models. The B17 tests run on the host Linux kernel as the reference behaviour. Every chapter states this in an untested box and in the Lab Verification paragraph.
   - Decide whether this is acceptable for the "real code runs" rule at L4, or whether the in-kernel ports must be built (a large job) before release.
3. **Kernel-mode shell for the serial transcript test (F3-35).** The P exam form (a scripted serial session against a reference transcript) is demonstrated with `kshell` in kernel mode. Decide whether the exam must require a user-mode shell.
4. **Pass codes counted as success.** `build.py`'s `run_ok` accepts any log with "exit code:". Expected failures are documented in `note:` lines; these are e2fsck exit 4, watchdog exit 3, timeout 124, the bcache self-check exit 1, and isa-debug-exit 33 as pass. Consider a formal `expected:` field in the log format so the builder can distinguish expected failures from real ones.
5. **"fsck is angry" design.** The course card asks for "an ext2 image after a test run; find the bitmap-update ordering bug". It is implemented as a power-cut harness with a driver whose block-bitmap update was moved after the directory entry, then a second run without fsck. The evidence pack is the harness write log, e2fsck's multiply-claimed report and debugfs. The answer key contrasts it with the correct order's leak-only report. Please confirm this matches the intended exercise.
6. **Forensic faults are injected by scripts.** Each fault is injected by `sed` or a small Python script in `run.sh`, so the answer is not visible in the listing files. The answer keys describe the injection.
7. **128-byte inodes in the ext2 lab** (`mke2fs -I 128`). These keep the driver small. mke2fs warns that they cannot handle dates beyond 2038 and are deprecated; the chapter mentions the warning. A 256-byte-inode variant could be a later extension.
8. **`minish` deviation.** `$?` is expanded when the line is read (documented in the source and the chapter). Decide whether to fix this before release; it is a lab extension now.
9. **The FAT32 driver writes long-name entries for case-only names** instead of using the byte-12 case flags. The chapter documents this as an unverified difference.
10. **New sources named** (all title only, gate G1):
    - Card, Ts'o and Tweedie;
    - Poirier;
    - Carrier, "File System Forensic Analysis";
    - Ganger, McKusick, Soules and Patt, "Soft Updates";
    - Plauger, "The Standard C Library";
    - the System V psABI and AAPCS64;
    - dosfstools and e2fsprogs manual pages;
    - the Linux-NTFS documentation.

    Several are tier 2 or 3 and are not yet in the guide's source registry. Please add them, or replace them with registry items.
11. **Glossary qualifiers.** These avoid merging with other courses' meanings: "Shared memory (between processes)", "Event (synchronisation object)", "Pipeline (shell)", "Directory entry (ext2)", "Directory entry (8.3 short name)" and "Signal (POSIX)". "Inode", "Superblock", "Semaphore", "Shell" and "Block device" are unqualified, and will merge with any identical term from other courses.
