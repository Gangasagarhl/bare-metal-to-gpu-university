# OS401 File systems in breadth and crash consistency — author notes

Author / Lab Engineer run, build of 2026-10-09 (no internet; batch brief `AUTHOR_BRIEF.md` + `AUTHOR_BRIEF_UPPER.md`).
Level L4, faculty F3 (analogy world: the school; registered analogy: file system = library catalogue
and shelves, "must survive a power cut in the middle of shelving").
Card: chapters F3-43 … F3-49, labs FS1–FS6, forensic "Lost after power cut" (F3-45), exam P "explain an
e2fsck report on a provided image" (built on the F3-44 forensic lab), project "FS2 passing 500 power-cut
runs" (F3-45 mini-project). Milestone texts (goal, read, build, accept) are quoted verbatim from
`SYSTEMS_CURRICULUM.html` section 7 in each chapter's Lab section and tagged C1.

## Files

- Chapters: `F3-43.html` … `F3-49.html` (all 21 sections + Jargon box + Transition box + Answers, forensic
  answer key inside Answers as `<ID>-forensic-key`; two inline SVG figures per chapter).
- Glossary: `glossary.json` — 63 four-part entries generated from the chapters' Jargon boxes (so the two
  never disagree); every `#gl-…` link in the chapters resolves to one of them. Four terms also exist in
  other courses with the same spelling and are merged by `build.py`: Crash consistency (OS201, OS304),
  Copy-on-write (COW) (OS201), Indirect block (OS304), Hard link (OS201). The Integrator should check
  the merged wording (guide 10.3).
- Labs: `university/labs/F3-43` … `F3-49` (sources, `run.sh`, `.out`, `.log`). Shared helpers:
  `F3-43/lablib.sh` (build flags, run records) is sourced by every OS401 lab; F3-45 reuses the F3-44
  headers `blockdev.h`, `ext2w.h`, `stream.h`, `workload.h`.

## Toolchain (from the run records)

`g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0` with
`-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined`; Python 3.13.16;
e2fsprogs 1.47.0 (5-Feb-2023: mke2fs, e2fsck, debugfs, dumpe2fs); strace 6.8; QEMU 8.2.2 with its
bundled SeaBIOS (TCG, no KVM); nasm; mkfs.fat 4.2 (2021-01-31); GNU mtools 4.0.43;
`blkid from util-linux 2.39.3 (libblkid 2.39.3, 04-Dec-2023)`; file-5.45. Linux x86_64 build container.
Not available: Windows, exfatprogs, ntfs-3g/mkntfs, xorriso, OVMF, root (no loop mounts), any real disk.

## Listings run (all with `university/labs/run_lab.sh university/labs/<ID>` from the repository root; every lab passes, exit 0)

"Expected-fail" = a run whose non-zero exit or wrong result is the point (forensic evidence or a
refusal); its `.log` says so in a `result:` line. "Untested on hardware" = ran only in emulation or as a
host program.

| Chapter | Runs | Result |
|---|---|---|
| F3-43 | `atomic_replace.cc` build; `crashsim.cpp` (5 strategies, every crash state enumerated); `strace_safe`, `strace_unsafe` | pass. Crash model is a program, not a disk: untested on hardware |
| F3-44 | build; mkfs; `stress` (10,000 ops, 540,226 writes); `fsck_stress` (e2fsck clean); `contents` (540 files, 0 different); `workload`; `stream_careful`/`stream_careless`; `harness_careful` (500 cut points, 0 damage, 9,787 promises, 0 wrong) | pass |
| F3-44 | `harness_careless` (exit 3, 65 of 100 damaged); `forensic_fsck` (e2fsck exit 4); `forensic_stream`, `forensic_debugfs` | expected-fail (forensic / exam P evidence) |
| F3-45 | build; mkfs; `linuxside` (debugfs replays our journal: "2 transaction(s) replayed, 2 block(s) written, 1 revoked"); `workload`; `stream_journal`; `harness_random` (500 runs, 0 lost, 0 repairs); `harness_commits` (200 cuts around commits) | pass. FS2's "Linux stopped abruptly" interchange: e2fsprogs stood in for the kernel; untested on hardware |
| F3-45 | `forensic_harness` (exit 3, 54 promises lost in 11 runs); `forensic_stream` (no flush before the commit block); `forensic_fsck` (e2fsck exit 4) | expected-fail ("Lost after power cut") |
| F3-46 | build; mkfs; info; tree (2,012 entries = host); extents; lookup (htree, matches debugfs) | pass |
| F3-46 | `refuse` (inline_data refused); `corrupt` (checksum mismatch, e2fsck agrees); `forensic_tree` | expected-fail |
| F3-47 | build; mkfs (+ blkid, file); rw (non-ASCII name round trip); check; stress (10,000 ops, 913 files, 0 different, 0 problems); frag (FAT chain) | pass |
| F3-47 | `blkid_checksum` (blkid and our tool both reject one changed byte); `forensic` (exit 4, 6 problems) | expected-fail |
| F3-48 | build (incl. nasm); mkiso; read (Rock Ridge and Joliet views = host); probe (blkid, file); boot (catalog dump, UEFI FAT image extracted, mtools lists it) | pass |
| F3-48 | `qemu_boot` (SeaBIOS boots the El Torito image, QEMU exit 33 = success) | pass, **untested on hardware** (emulation only) |
| F3-48 | `checksum_tolerance` (zeroed validation checksum: isoread says INVALID, SeaBIOS still boots) | observation, **untested on hardware** |
| F3-48 | `forensic_catalog`; `forensic_qemu` (exit 124: hang after "Booting from 0000:7c00") | expected-fail, **untested on hardware** |
| F3-49 | build; mkimg (+ blkid, file); info + check (0 problems); ls (159 entries = host), find traces, sparse cat; records | pass |
| F3-49 | `torn` (ls exit 3, check exit 4, record 0 from `$MFTMirr`); `dirty` (warning, image unchanged) | expected refusals (pass) |
| F3-49 | `forensic` (exit 0 with four wrong names and a wrong file: the point) | expected-fail |

No CUDA/HIP. No listing was run on real hardware: no disk, card, stick, optical drive or power cut.

Late changes after the first full run (each lab rerun afterwards, passing): F3-48 `run.sh` now records
the real `mkfs.fat 4.2` version in `mkiso.log` (it recorded the usage line before); F3-49 `ntfsread.cc`
gained two refusals (compressed/encrypted `$DATA`, `$ATTRIBUTE_LIST`) — **untested**, no image contains
either.

## Claims that could not be verified (sources title only, gate G1 open)

No source document could be opened. Every technical claim is tagged to D-entries in each chapter's
Sources, all marked "Title only — not opened during this build". Main sources the Source Researcher must
confirm (edition, sections):

- F3-43: OSTEP (crash consistency, FSCK and journaling chapters); NVMe base spec (Flush, FUA, volatile
  write cache); VIRTIO spec (block flush); POSIX (fsync, rename); Pillai et al. (OSDI 2014,
  application-level crash consistency); QEMU block cache documentation.
- F3-44: Poirier "The Second Extended File System"; e2fsprogs man pages (e2fsck exit codes, debugfs);
  OSTEP; Linux kernel ext4 documentation for shared ext2 fields.
- F3-45: Linux kernel documentation "ext4 Data Structures and Algorithms", journal (JBD2) section;
  e2fsprogs; Prabhakaran et al. / OSTEP for ordered mode.
- F3-46: Linux kernel ext4 documentation (superblock, group descriptors, extents, htree, metadata_csum,
  inline data); Intel SDM / Arm ARM for CRC32 instructions.
- F3-47: Microsoft "exFAT file system specification"; SD Association Physical Layer and File System
  specs; libblkid's exFAT probe as tier 4.
- F3-48: ECMA-119; Joliet specification; IEEE P1281/P1282 (SUSP/RRIP); El Torito 1.0; ECMA-130 and MMC;
  SeaBIOS and QEMU documentation.
- F3-49: Linux-NTFS documentation (Russon, Fledel); Carrier "File System Forensic Analysis";
  Windows Internals Part 2.

Independent confirmation that *was* obtained in this build (tools, not documents): e2fsck/debugfs/
dumpe2fs accept our ext2 writer's images (F3-44), replay our JBD2 journal (F3-45) and agree with our
ext4 reader (F3-46); libblkid accepts our exFAT boot region, boot checksum and label (F3-47), our ISO
descriptors, Joliet descriptor and boot record (F3-48), and our NTFS boot sector and volume label
(F3-49); file(1) recognises all three; mtools reads the El Torito UEFI FAT image; SeaBIOS boots the
El Torito BIOS entry. Everything else in F3-47 … F3-49 is our code agreeing with our own code.

## Unverified and untested-on-hardware boxes (AH-18), per chapter

- F3-43 (5): device flush/FUA/atomicity semantics; fsync/rename/POSIX statements; QEMU cache modes and
  killed-QEMU behaviour; hardware (no real write cache, no power cut); lab (crash model is a program).
- F3-44 (4): ext2 offsets/rules from memory; not run in a kernel, no QEMU kill, FS1's ">4 GiB and sparse"
  clause only partly covered (no triple indirect); hardware flush semantics; lab in-kernel/QEMU parts.
- F3-45 (4): JBD2 offsets, block types and tag flags from memory; not tested against the Linux kernel
  (FS2 interchange done with e2fsprogs only); hardware (Linux preflush/FUA, async commit); lab.
- F3-46 (3): ext4 offsets and checksum rules from memory (cross-checked with e2fsprogs only); hardware
  CRC instructions; lab (not in a kernel, inline_data not implemented, tree made with `mkfs.ext4 -d`).
- F3-47 (3): every exFAT offset from memory, only boot region/checksum/label confirmed by libblkid;
  hardware (SD/flash claims); lab (FS4 acceptance — Windows, exfatprogs, fsck.exfat — not run;
  minimal 128-entry up-case table; reads only its own volumes).
- F3-48 (4 + 1 warning): ECMA-119 offsets (path tables unconfirmed); SUSP/RRIP/Joliet formats (no CE,
  TF or relocation; Linux isofs and xorriso not used); hardware (SeaBIOS under TCG only, no UEFI boot
  tried, no OVMF); lab (FS5 acceptance — xorriso, installer image — not run; no UDF). Warning box:
  SeaBIOS booting with a bad catalog checksum is an observation, not a rule.
- F3-49 (4): NTFS offsets from memory, **generator and reader by the same author** (shared misreadings
  invisible); Fast Startup / `$LogFile` behaviour; hardware (4Kn strides, atomicity; damage simulated);
  lab (FS6 acceptance — Windows-made image — not run; no attribute lists, compression, DOS names,
  real `$UpCase`, `$Secure` or `$LogFile`).

## Decisions for the owner

1. **Approve publication with unverified boxes** (AH-19): every chapter has them; F3-47 … F3-49 rest on
   formats checked only by libblkid/file and our own tools.
2. **Acceptance tests that need tools this build lacked** — owner to provide an environment with:
   a Linux kernel that can mount images and a QEMU-kill harness (FS1, FS2 interchange and power cuts);
   exfatprogs and a Windows VM (FS4); xorriso and a current installer ISO, plus OVMF for the UEFI entry
   (FS5); a Windows VM to make an NTFS test image (FS6). Until then the chapters say plainly which
   clauses were not run.
3. **UDF**: the course card's goals say "ISO 9660/UDF"; FS5 makes UDF optional. F3-48 does not cover
   UDF (mentioned only as the mini-project's optional extension). Decide whether a UDF section or a
   separate chapter is wanted.
4. **Course project scale**: "FS2 passing 500 power-cut runs" is met in this build by 500 replayed
   cut points of a recorded write stream (F3-45 `harness_random`), not by 500 killed QEMU instances. The
   mini-project rubric asks for the QEMU version; confirm that this is the intended bar.
5. **Exam P image**: the F3-44 forensic image (careless writer, e2fsck exit 4, "deleted/unused inode
   12") is designed to be the exam's "provided image"; the Exam Writer should regenerate a fresh one
   with another seed so answers cannot be copied from the chapter.
6. **ext2 writer gaps** (F3-44): no triple indirect blocks, no rename over an existing name, no
   directory rename/rmdir, no long symlinks. FS1 mentions files above 4 GiB; decide whether that clause
   must be met in the chapter's own code or left to students.
7. **NTFS generator**: `ntfsgen.cc` exists only because no Windows image was available. Once one is,
   replace the generated image in the lab with a checked-in Windows-made image (small, licence-clean)
   and keep the generator only for the damage experiments.
8. **Analogy registrations (proposals to the Dean, guide 8.2)** — used in the chapters and marked
   "proposed" in each meta comment:
   - journal = the librarian's day book (F3-43, F3-45); commit block = the tick under a finished entry
     (F3-45); flush = the porter's signed delivery note (F3-43); write stream = the porter's delivery
     list (F3-44);
   - extent = a shelf range written on one card; checksum = the seal on each catalogue drawer (F3-46);
   - allocation bitmap = the free-place chart; NoFatChain = a card saying "all volumes stand side by
     side from place 5" (F3-47);
   - ISO 9660 = the printed, bound catalogue that is never changed; Rock Ridge and Joliet = margin notes
     for two kinds of reader; El Torito = the caretaker's "start here" page (F3-48);
   - NTFS = the neighbouring school's library with unpublished rules; MFT = its master register;
     update sequence = the form number at the foot of every page of a two-page form (F3-49).
   Each chapter lists where its analogy breaks.
9. **Length**: chapters are about 5,000–7,700 words including jargon box, tables, answers and keys
   (F3-43 is the longest because it carries the course's crash model). Within L4 range per the brief,
   but the Editor may want to trim F3-43.

## Cross-links used

`#OS304`, `#OS401`, `#F3-31`, `#F3-32`, `#F3-33` (OS304: VFS, initial RAM disk and block cache; FAT32;
ext2), `#F3-43` … `#F3-49`, `../SYSTEMS_CURRICULUM.html#fs`, `#gl-…` (own glossary). `python3
university/build/build.py` reports no PROBLEM line mentioning F3-43 … F3-49 or OS401.
