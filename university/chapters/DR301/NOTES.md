# DR301 — Drivers I: PCI, serial, virtio and storage: author notes

Chapters F4-01 to F4-07, level L3, 5 credits. Prerequisites: OS302 (B4, B7). Maps to Track C, milestones C1 to C6.
Labs are in `university/labs/F4-01` to `university/labs/F4-07`.

Each lab folder passes `university/labs/run_lab.sh university/labs/<ID>` with status 0. All seven were run again, one after the other, at the end of this build on 2026-10-09, after the last code change. The table below is generated from the `.log` files of that final sweep. Timing and throughput figures (milliseconds and KiB/s of emulated QEMU TCG time) change from run to run. The chapters label them as emulated and do not quote them in prose; the worked examples use deterministic runs only.

## Toolchain (as recorded in the logs)

- **QEMU:** 8.2.2 (Debian 1:8.2.2+ds-0ubuntu1.18), `qemu-system-x86_64`, with SeaBIOS.
  - `pc` machine for F4-01 to F4-04 (F4-02 also uses q35); `q35` for F4-05 to F4-07.
  - **TCG only.** The build container has no KVM.
- **Compiler:** g++ 13.3.0 (Ubuntu 13.3.0-6ubuntu2~24.04.1); GNU ld 2.42; gcc 13.3.0 for `linux_values.c`.
- **Host tools:** Python 3.13; tshark 4.2.2 (F4-05); Linux UAPI headers from linux-libc-dev 6.8.0-146.146 (F4-01, F4-02, F4-05 layout checks).
- **Lab kernel:** a 32-bit Multiboot kernel built from shared sources (`F4-01/lablib.sh`: `rec`, `kbuild`, `qboot`, `hostbuild`; `F4-01/kernel.ld`).
  - Paging is off, so physical = virtual.
  - Interrupts use the legacy 8259 PIC, with the PIT at 1 kHz as the millisecond clock.
  - Flags: `-m32 -std=c++20 -ffreestanding -fno-exceptions -fno-rtti -O2 -Wall -Wextra -Wpedantic -Werror` and related.
- **Host tests:** `-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined`.
- **QEMU exit codes:** `isa-debug-exit` at port 0xf4. The kernel writes 0x10 for pass (QEMU exit code 33) and 0x01 for fail (exit code 3).

## Listings run

**Nothing was run on real hardware or under KVM.** Every kernel run is QEMU 8.2.2 TCG, and every host program was built with ASan and UBSan. Status means:

- **pass:** the expected result.
- **expected-fail:** a failure the lab is designed to produce and checks for (exit code 3 from a forensic kernel, or a compile error that must happen).
- **untested on hardware:** applies to every QEMU run.

| Chapter | Run (`.log`) | Exit / result | Status |
|---|---|---|---|
| F4-01 | boot | 33 | pass; untested on hardware |
| F4-01 | boot2 | 33 | pass; untested on hardware |
| F4-01 | build | 0 | pass |
| F4-01 | forensic | 33 | pass; untested on hardware |
| F4-01 | reg_ro_write | compile error | expected-fail (must not compile) |
| F4-01 | regcheck | 0 | pass |
| F4-01 | regs_layer | 0 | pass |
| F4-02 | bar_math | 0 | pass |
| F4-02 | build | 0 | pass |
| F4-02 | compare | 0 | pass |
| F4-02 | enum | 33 | pass; untested on hardware |
| F4-02 | enum_pc | 33 | pass; untested on hardware |
| F4-02 | forensic | 1 | pass |
| F4-02 | pcicheck | 0 | pass |
| F4-02 | qemu_pci | 0 | pass; untested on hardware |
| F4-03 | build | 0 | pass |
| F4-03 | forensic | 0 | pass; untested on hardware |
| F4-03 | keyboard | 33 | pass; untested on hardware |
| F4-03 | paste | 33 | pass; untested on hardware |
| F4-04 | build | 0 | pass |
| F4-04 | forensic | 0 | pass; untested on hardware |
| F4-04 | rtc_decode | 0 | pass |
| F4-04 | rtc_fixed | 33 | pass; untested on hardware |
| F4-04 | rtc_host | 33 | pass; untested on hardware |
| F4-05 | build | 0 | pass |
| F4-05 | capture | 0 | pass |
| F4-05 | forensic | 3 | expected-fail (forensic evidence); untested on hardware |
| F4-05 | image | 0 | pass |
| F4-05 | virtio | 33 | pass; untested on hardware |
| F4-05 | virtio_check | 0 | pass |
| F4-06 | ahci | 33 | pass; untested on hardware |
| F4-06 | ahci_trace | 33 | pass; untested on hardware |
| F4-06 | boottrace | 0 | pass; untested on hardware |
| F4-06 | build | 0 | pass |
| F4-06 | image | 0 | pass |
| F4-07 | build | 0 | pass |
| F4-07 | forensic | 3 | expected-fail (forensic evidence); untested on hardware |
| F4-07 | identify | 0 | pass |
| F4-07 | image | 0 | pass |
| F4-07 | nvme | 33 | pass; untested on hardware |
| F4-07 | nvme_trace | 33 | pass; untested on hardware |
| F4-07 | root | 0 | pass |

## Acceptance tests C1–C6 and their honest status

The acceptance-test wording is quoted verbatim in each chapter's Lab section. Summary:

- **C1 (F4-02) and C2 (F4-03):** the tests named in the chapters pass in QEMU. See the chapters for the exact wording and runs.
- **C3 (F4-04):**
  - Kernel time against host time: pass, within 2 s, checked by `rtc_stamp.py`, which stamps the host clock when the kernel's first `date:` line arrives.
  - The fixed QEMU start date: pass (`-rtc base=2026-12-31T23:59:58`, including the rollover).
- **C4 (F4-05):**
  - "Random-read and random-write test over 1 GiB with checksum, with queue depth 1 and 32": pass.
    - 4000 operations per depth, uniformly chosen over the 262144 blocks of 4 KiB.
    - Each read is compared in full, not by checksum.
    - The host replays the generator and checks the image (`verify_image.py`).
  - "virtio-net: ARP request … appears in a QEMU packet capture opened in Wireshark": pass. The capture is read with tshark 4.2.2 (Wireshark's command-line program), not the Wireshark GUI.
  - "FAT32 (B15) and ext2 (B16) tests run on virtio-blk": **not done.** The lab kernel has no block layer or file systems. The chapter says so in a "Not tested in this build" box, and the mini-project asks for it.
- **C5 (F4-06):**
  - IDENTIFY against QEMU's configuration: pass. Model and serial come from `-device ide-hd,model=…,serial=…`; the capacity is the 1 GiB image.
  - "The same 1 GiB random I/O test as C4 passes on AHCI": **partly.** It passes at queue depth 1 only, because the driver uses one command slot. Depth 32 needs NCQ, which is the mini-project.
  - Throughput logged next to virtio-blk: logged (emulated KiB/s). The comparison in the design log is the student's lab step 3.
- **C6 (F4-07):**
  - Identify against the configuration: pass (`identify` step).
  - "1 GiB random I/O test passes with 1, 4 and 8 queues on 8 CPUs": **partly.** It passes with 1, 4 and 8 queue pairs on one CPU. Per-CPU submission on 8 CPUs was not tested.
  - "Your root file system mounts from NVMe": **partly.** A read-only USTAR archive on namespace 2 is walked by the kernel and compared file by file with the host (`root` step). It is not a mounted, writable file system. The course project asks for an ext2 root mounted through the student's VFS.

## Unverified boxes (all D-sources are "title only — not opened during this build", dossier gate G1 open)

The only sources opened in this build are:

- the project's own `SYSTEMS_CURRICULUM.html`;
- the project's own `DRIVERS.html`;
- the Linux UAPI headers installed in the container. These were compiled against and compared, and are an implementation, not a specification.

- **F4-01:** two boxes.
  - Linux and Windows driver-model names are from memory of LDD3 and Windows Internals.
  - 16550 register offsets and bits are from memory. They agree with `<linux/serial_reg.h>` (33 values, 0 differences). PC port numbers are by convention.
- **F4-02:** one box plus one warning.
  - PCI header offsets, command bits, BAR bits and capability IDs agree with `<linux/pci_regs.h>` (28 values, 0 differences).
  - The ECAM layout, extended capability header, MCFG layout and RSDP search area are confirmed only by QEMU.
  - The warning covers command/status writes through a 32-bit access.
- **F4-03:** one box plus one warning.
  - 8259 initialisation, PIT, i8042 bits and 16550 FIFO encoding are from memory and confirmed only by QEMU.
  - The warning is the intermittent paste stall (see open issues).
- **F4-04:** two boxes plus one warning.
  - RTC registers and bits, FADT "FACP" and the century field at offset 108, and NMI gating are from memory.
  - The crystal frequency, CMOS size and UEFI service names are from memory.
- **F4-05:** four boxes.
  - Virtio capability offsets, status bits, feature bits, device IDs and handshake order are from memory. They agree with the Linux UAPI headers (38 values, 0 differences, `virtio_check`). The handshake order and the FEATURES_OK read-back rule are from memory only.
  - The x86 ordering argument and Linux's barrier placement are from memory. Ring alignment rules, the meaning of available-ring flag bit 0 and the meaning of used-element `len` are from memory. TCG on one CPU cannot test ordering.
  - KVM exits on notify, vhost and hardware virtio: general knowledge.
  - Not tested: the FAT32/ext2 acceptance test.
- **F4-06:** four boxes.
  - **No second implementation was available.** All AHCI offsets and bits, the start/stop order, the COMRESET minimum and the PxSSTS fields are confirmed only by QEMU's ich9-ahci model and its trace naming the registers.
  - ATA status and error bits and the IDENTIFY word numbers are from memory. The IDENTIFY words are indirectly confirmed by model, serial and capacity matching. The claim that a real disk reports a read past the end differently is an expectation.
  - SATA out-of-band (COMRESET/COMINIT) is from memory. No timings were taken on real disks.
  - Not tested: depth 32 on AHCI, and anything on physical hardware.
- **F4-07:** three boxes.
  - **No second implementation was available.** All NVMe register offsets, CAP/CC fields, SQE/CQE layouts, opcodes, CNS values and Identify byte offsets are confirmed only by QEMU's nvme model, its `pci_nvme_*` trace and Identify matching the configuration.
  - Also from memory only: the claims that queue creation fails without IOSQES/IOCQES, and that the specification treats a CQ head doorbell beyond posted entries as invalid. The forensic key marks the latter as unverified.
  - SSD firmware (flash translation, wear levelling, flush semantics) is general knowledge.
  - Not tested: 8 CPUs and a mounted root.
- **Forensic inferences marked as such in the keys:**
  - F4-05: the mechanism by which QEMU completes a read with no device-writable data buffer with status 0 is inferred, not read in QEMU's source. The used-ring `len` value of 1 is a prediction; the lab does not print it.
  - F4-06: whether Windows' `amdsata` init time covers its port scan is unknown (Windows Internals not opened).

## Decisions for the owner

1. **The lab kernel is 32-bit with paging off and uses the 8259 PIC, not the IOAPIC or MSI.**
   - This keeps every DMA address physical = virtual and reuses one small kernel for seven labs.
   - The student's OS302 kernel is 64-bit with paging. The chapters say where a real kernel differs (physical addresses for DMA, MSI-X in C7).
   - Alternative: build the labs on the OS302 lab kernel instead. That needs coordination with OS302's author.
2. **The random I/O tests run 4000 operations per configuration over a 1 GiB address range, not every block of 1 GiB.**
   - Each read is compared in full, and the host checks the whole written set plus a sample of unwritten blocks.
   - A full 1 GiB pass under TCG would take minutes per run.
   - Please confirm this meets "random I/O test over 1 GiB" or set a required operation count.
3. **All drivers poll; there are no interrupts for virtio or NVMe.** AHCI uses the PIC line. C6 allows polling until C7, and C4 and C5 do not mention interrupts for virtio. MSI-X is left to DR302 (C7).
4. **C6 runs on one CPU.** "1, 4 and 8 queues on 8 CPUs" is met for queue counts but not for CPUs. The course project asks for it on the student's SMP kernel.
5. **The C6 and course-project root is a read-only USTAR archive, not a mounted file system.** It is checked file by file against the host's `tarfile`. The project rubric requires ext2 mounted through the student's VFS.
6. **The course forensic ("the slow boot driver") uses a boot-trace CSV produced by the lab kernel itself, not a Windows ETL trace.**
   - It comes from two QEMU boots (healthy port probe against reset-every-port with a 1 s timeout), formatted like the driver tables of `DRIVERS.html`.
   - The chapter says so in a note box, and connects it to `DRIVERS.html` (List 1, `amdsata` 0.4 → 0.4 ms, `kernel_to_smss`) and to curriculum 4.3 (amdsata/storahci → C5).
   - If the owner wants a real ETL-derived CSV, one must be supplied.
7. **C4's FAT32/ext2-on-virtio test was not run.** It depends on the student's B14–B16 work (F3-31 to F3-33, OS304), which is not a DR301 prerequisite.
   - The chapter's prerequisites line names OS304 for that test only.
   - Consider adding OS304 as a co-requisite, or moving that acceptance test to the course project.
8. **AHCI depth 32 (NCQ) is a mini-project, not in the lab.**
9. **No spare-PC runs.** Every "optional spare PC" path is untested, and every chapter and log says "untested on hardware".
10. **QEMU's file chardev lost bytes during F4-03's development.** Feeding the paste test from a file chardev lost input bytes, so the paste test uses a socket chardev with a Python feeder (`paste_feed.py`) instead. F4-03 names the socket chardev but does not discuss the file-chardev loss. The loss was observed and not investigated, so it could be a QEMU limitation or a usage error.

## Open issues

- **F4-03 paste stall (intermittent).** One stall happened in about ten runs, during an earlier `run_lab.sh` sweep while other agents' emulators loaded the machine. It could not be reproduced: three normal runs and three under deliberate CPU stress all passed.
  - Mitigation: `run.sh` retries the paste step once and records the failed attempt in `paste.out` ("attempts: N" in the log), and the kernel prints IER/LSR/MCR after 10 s without a byte.
  - The cause is unknown. Candidates: a race in the socket set-up, or a lost receive interrupt.
- **F4-06 driver flaw, documented and left as lab step 4:** the AHCI interrupt handler and the polling issue path both consume PxIS (RW1C). The error line shows `PxIS=0`, and the error is caught through PxTFD.ERR.
- **F4-06 and F4-07 traces:** SeaBIOS uses both controllers before the kernel. The run scripts split the trace into a firmware part and a kernel part (AHCI: from the last GHC write with HR set; NVMe: from the last enable). The F4-06 split was added in the final round.

## Analogy proposals (F4: the school and its visitors)

These extend the faculty analogy. Please accept, reject or rename them.

- **Sign-in book:** PCI configuration space and bus enumeration (F4-02).
- **The interpreter's first greeting:** probe (F4-01).
- **The secretary:** the 8259 PIC, deciding which bell to pass on (F4-03).
- **The hall clock and the principal's stopwatch:** the RTC and the monotonic clock (F4-04).
- **Delivery notes, order board, receipt book, phoning the depot:** virtio descriptors, available ring, used ring and notification (F4-05). The delivery truck stays DMA.
- **The loading dock with 32 numbered bays; shouting at a gate:** AHCI command slots; COMRESET (F4-06).
- **Order and receipt trays with bells; this round's ink colour:** NVMe SQ/CQ with doorbells; the phase tag (F4-07).
  - Note the collision: the faculty uses "doorbell" for interrupts (device to CPU), while NVMe's "doorbell register" is CPU to device. The jargon box and the glossary entry say so, as HW205's entry does.

## Build check

`python3 university/build/build.py` was run after the final sweep. Its PROBLEM lines that mention DR301 chapters are listed below, together with what was done about them.

Result of the final run (2026-10-09): exit 0. One DR301 problem was reported, "duplicate id: gl-end-of-interrupt-eoi". It came from F4-03's term "End of interrupt (EOI)" slugging to the same id as HW204's "End-of-interrupt (EOI)". F4-03 and DR301's glossary now use HW204's spelling, so the two entries merge. After that, no PROBLEM line mentions a DR301 chapter, link or glossary entry. The remaining PROBLEM lines are broken `#gl-` links in other courses' chapters.
