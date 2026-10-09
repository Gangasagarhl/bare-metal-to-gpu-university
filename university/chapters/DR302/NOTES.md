# DR302 — Drivers II: interrupts, IOMMU, USB, networking, ACPI: author notes

Chapters F4-08 to F4-13, level L4, 6 credits. Prerequisites: DR301, OS303. Maps to Track C, milestones C7–C11 and C13 (F4-13 is optional, as C13 is).
Labs are in `university/labs/F4-08` to `university/labs/F4-13`.

Each lab folder passes `university/labs/run_lab.sh university/labs/<ID>` with status 0. All six were run again, one after the other, at the end of this build on 2026-10-09, after the last code change. The table below is generated from the `.log` files of that final sweep. Timings, packet counts at the edges of the TCP runs and the exact length of the audio recordings change from run to run; the chapters say "about" where a number varies, and the worked examples use deterministic values only.

## Toolchain (as recorded in the logs)

- **QEMU:** 8.2.2, `qemu-system-x86_64 -machine q35`, SeaBIOS, **TCG only** (no KVM in the build container).
  - Devices used: `edu`, `qemu-xhci`, `usb-kbd`, `usb-mouse`, `usb-storage`, `e1000` with user-mode networking (slirp), `intel-iommu`, `intel-hda` + `hda-output` with `-audiodev wav`, `tpm-tis` with the `emulator` backend.
- **Compiler:** g++ 13.3.0, GNU ld 2.42 (same flags as DR301; the lab kernel is DR301's 32-bit Multiboot kernel, paging off).
- **Host tools:** Python 3 (numpy for F4-12), tshark 4.2.2 (F4-09, F4-10), dosfstools and mtools (F4-09), Linux UAPI and glibc headers for layout checks.
- **Shared code:** every DR302 kernel links DR301's `F4-01` (boot, `kbase`, `lablib.sh`) and `F4-02` (PCI, ACPI) sources, and F4-09 to F4-13 link F4-08's interrupt, MSI, VT-d and DMA layer (`isr256.S`, `intr.cc`, `msi.cc`, `vtd.cc`, `dma.cc`). **The DR302 labs therefore depend on the DR301 lab folders being present and unchanged.** A change to `F4-01/lablib.sh` or `F4-02/pci.cc` must be followed by a rerun of all twelve labs.
- **QEMU exit codes:** `isa-debug-exit`: 33 = pass, 3 = the kernel reported FAILED. F4-11's `acpi` run exits 0 because the guest powers the machine off.

## Listings run

**Nothing was run on real hardware or under KVM.** Every kernel run is QEMU 8.2.2 TCG. Status means:

- **pass:** the expected result.
- **expected-fail:** a failure the lab is designed to produce and `run.sh` checks for (a forensic kernel reporting FAILED, or a host check rejecting forensic evidence).
- **untested on hardware:** applies to every QEMU run.

| Chapter | Run (`.log`) | Exit / result | Status |
|---|---|---|---|
| F4-08 | build | 0 | pass |
| F4-08 | forensic | 33 | pass; untested on hardware |
| F4-08 | iommu | 33 | pass; untested on hardware |
| F4-08 | msi | 33 | pass; untested on hardware |
| F4-08 | msicheck | 0 | pass |
| F4-08 | vtd_decode | 0 | pass |
| F4-09 | build | 0 | pass |
| F4-09 | capture | 0 | pass |
| F4-09 | forensic | 3 | expected-fail; untested on hardware |
| F4-09 | forensic_capture | 0 | pass |
| F4-09 | image | 0 | pass |
| F4-09 | usb | 33 | pass; untested on hardware |
| F4-09 | usb_iommu | 33 | pass; untested on hardware |
| F4-09 | usbcheck | 0 | pass |
| F4-10 | build | 0 | pass |
| F4-10 | capture | 0 | pass |
| F4-10 | forensic | 3 | expected-fail; untested on hardware |
| F4-10 | forensic_capture | 0 | pass |
| F4-10 | net | 33 | pass; untested on hardware |
| F4-10 | net_iommu | 33 | pass; untested on hardware |
| F4-10 | netcheck | 0 | pass |
| F4-11 | acpi | 0 | pass; untested on hardware |
| F4-11 | amlcheck | 0 | pass |
| F4-11 | build | 0 | pass |
| F4-11 | dsdt | 0 | pass |
| F4-11 | forensic | 3 | expected-fail; untested on hardware |
| F4-12 | audio | 33 | pass; untested on hardware |
| F4-12 | audio_iommu | 33 | pass; untested on hardware |
| F4-12 | build | 0 | pass |
| F4-12 | forensic | 33 | pass; untested on hardware |
| F4-12 | forensic_wav | 1 | expected-fail |
| F4-12 | wav | 0 | pass |
| F4-12 | wav_iommu | 0 | pass |
| F4-13 | build | 0 | pass |
| F4-13 | forensic | 33 | pass; untested on hardware |
| F4-13 | forensic_verify | 1 | expected-fail |
| F4-13 | hashcmp | 0 | pass |
| F4-13 | notpm | 33 | pass; untested on hardware |
| F4-13 | sha256check | 0 | pass |
| F4-13 | tpm | 33 | pass; untested on hardware |
| F4-13 | verify | 0 | pass |

## Acceptance tests and their honest status

- **C7 (F4-08).** IOMMU fault instead of corruption: met (fault record decoded, canary unchanged). "All storage and network tests pass with the IOMMU": met for DR302's USB storage, network and audio labs (each has an `_iommu` run); DR301's AHCI, virtio and NVMe labs were **not** rerun with `intel-iommu`. NVMe with one MSI-X vector per queue and per-CPU counters: **not done** (MSI-X is shown on qemu-xhci; the lab kernel runs on one CPU).
- **C8 (F4-09).** Enumeration of keyboard, mouse and stick with descriptors: met (mouse by hot-plug). Typing reaches the kernel, but there is **no shell**. The stick: FAT32 boot sector, root directory and a file read, one block written and read back; **the B15 tests were not run**. Hot-plug and unplug through QMP: met. Spare-PC keyboard: not done.
- **C9 (F4-10).** DHCP, ping of the gateway, HTTP fetch with matching bytes, 0 malformed packets, 100 MiB echo with matching SHA-256: met. Host-to-guest ping: **not possible** with slirp (it forwards TCP/UDP only). The echo server runs **in kernel space** (no user space or sockets in the lab kernel). The 5 % loss is induced **in the guest**, not by a QEMU filter (see decisions).
- **C10 (F4-11).** Power button through QMP `system_powerdown` → S5 → QEMU exits: met (no file systems to sync). Namespace device list printed: met; the iasl comparison was **not possible** (iasl not installed). **No AML interpreter** was ported; the curriculum says to port ACPICA or uACPI. Battery and thermal: not done.
- **C11 (F4-12).** The capture matches the source samples: met for a **generated tone**, not a WAV file read from disk by the kernel (every recorded frame identical; the last 30–40 ms missing from the recording). Widget graph dump: met, against a description from memory of the spec. "A simple audio device interface for user programs" (Build line): not done.
- **C13 (F4-13, optional).** PCR values equal the replay of the firmware log; GetRandom returns data; response codes decoded: met **with a mock TPM, not swtpm** (swtpm not installed). See "Mock TPM" below.

## Unverified boxes (all D-sources are "title only — not opened during this build", dossier gate G1 open)

Every chapter has two boxes: a "Not verified" box in Layer 3 and a "Not tested in this build" box in the Lab section (12 boxes in all).

- **F4-08:** VT-d register offsets, CAP/ECAP fields, root/context/second-level entry formats, fault-record layout and reason codes (from memory of the VT-d spec); MSI message format. Confirmed only by QEMU's intel-iommu behaving as expected. MSI capability offsets were checked against `<linux/pci_regs.h>` (24 of 24). Not tested: NVMe MSI-X per queue, per-CPU counters, DR301 labs under the IOMMU, interrupt remapping, page-selective invalidation, caching mode 1.
- **F4-09:** xHCI offsets, PORTSC bits, TRB types and fields, context layouts, endpoint types and interval rules (from memory of the xHCI spec); BOT, SCSI CDB and sense layouts; HID boot report. Confirmed by qemu-xhci and the packet captures; chapter-9 constants and SCSI opcodes checked against Linux headers. Not tested: hubs, mouse driver, B15, shell, BOT reset recovery, endpoint-halt recovery, firmware hand-off, real hardware.
- **F4-10:** e1000 registers, EEPROM protocol, legacy descriptors, MPC/RNBC statistics (from memory of the 8254x manual); TCP timer and congestion rules as remembered from the RFCs. Header layouts checked against glibc; wire format by tshark (0 malformed, 0 bad checksums). The claim that QEMU 8.2's network filters cannot drop packets is from memory. Not tested: DNS, sockets, user-space server, IPv6, e1000e, interrupt-driven data path, real NIC.
- **F4-11:** FADT offsets, PM1 bits, AML opcodes, PkgLength and NameString rules, _PRT and resource descriptor formats (from memory of the ACPI spec). Confirmed by QEMU/SeaBIOS behaviour and hand-made tests from the same memory. Not tested: AML interpreter, OS services layer, EC, GPEs, method evaluation, battery and thermal, iasl, any firmware other than SeaBIOS.
- **F4-12:** HDA register offsets, bit positions, verb and parameter IDs, payload layouts (from memory of HDA rev 1.0a). Confirmed by QEMU's intel-hda/hda-output and the sample-exact recording. The decoding of PCM-rates 0x000201fc and amp capability 0x80034a4a is unchecked. The explanation for the missing recording tail is an inference. Not tested: WAV from disk, user interface, continuous playback, input, unsolicited responses, EAPD, real codecs.
- **F4-13:** TIS registers, TPM 2.0 command/response layouts and codes, TPM2 table fields, event log structures, PCR conventions (from memory of the TCG specs); the swtpm control protocol in `mock_tpm.py` (from memory; QEMU accepted it); command 0x181 before Startup (probably QEMU's version probe); vendor 0x1014. Not tested: swtpm, real TPM, CRB, UEFI event log, quotes, other banks.

## Mock TPM: circular evidence (F4-13)

The build container has no swtpm, so `university/labs/F4-13/mock_tpm.py` plays the TPM behind QEMU's `tpm-tis` "emulator" backend. It was written by the same author, from the same memory, as the driver. The chapter's Layer 3 splits the evidence:

- **Independent:** the TIS register protocol (QEMU's real tpm-tis model); the event log (written by SeaBIOS) and its replay matching the PCRs that SeaBIOS's own extends produced; SHA-256 (FIPS examples and Python's hashlib on 2000 messages).
- **Partly independent:** the kernel's PCR_Extend layout equals what the mock parses, and the mock parses SeaBIOS's PCR_Extend correctly.
- **Circular:** PCR_Read and GetRandom response layouts, response codes, the behaviour before Startup.

**Recommendation:** rerun F4-13 with swtpm (`swtpm socket --tpm2 --ctrl type=unixio,path=…`) before publishing. `run.sh` needs only its `tpmboot` function changed.

## Decisions for the owner

1. **F4-10: the 5 % loss is induced in the guest** (a seeded drop of TCP segments by a hash of sequence number and retransmission count: receive side in `net.cc`, transmit side in `tcp.cc`), not by a QEMU network filter. From memory, QEMU 8.2's filters (`filter-dump`, `filter-buffer`, `filter-mirror`, `filter-redirector`, `filter-rewriter`, `filter-replay`) cannot drop a percentage of packets; this was not verified. Please accept or provide a host-side loss method (for example a TAP device with `tc netem`, which needs privileges the build container lacks).
2. **F4-10: host-to-guest ping is impossible with slirp**; the chapter shows TCP through `hostfwd` instead. A TAP or socket network backend would allow it.
3. **F4-10: the echo server runs in the kernel**, with no sockets and no DNS; the acceptance test says "user space". This follows from the DR302 lab kernel having no user mode. Either accept, or move the user-space part to the course project.
4. **F4-10: RTO minimum and TIME_WAIT are 200 ms**, far below RFC 6298's 1 s minimum and the usual 2 MSL, so that the lab finishes in minutes under TCG. The chapter labels both as lab choices.
5. **F4-11: no AML interpreter.** The lab is a declaration-only walker and a static `_PRT` reader; porting ACPICA or uACPI is the mini-project. iasl was not available, so the namespace was not cross-checked with a disassembly.
6. **F4-09: no hub driver and no B15 tests**; the stick is read at the FAT32 level by the lab's own minimal reader. **No firmware hand-off** (USB legacy support capability): with the IOMMU on, a DMAR read fault at 0x3fdec60 appeared when translation was enabled. The likely cause is SeaBIOS's xHCI rings still running; this was not proven.
7. **F4-08: NVMe MSI-X per queue is not done**; MSI-X is demonstrated on qemu-xhci. The NVMe part needs DR301's F4-07 driver extended. Consider making it the F4-08 mini-project (it is lab step 6).
8. **F4-12: the source is a generated tone**, not a WAV file read by the kernel (the lab kernel has no file system). The sample comparison is stronger evidence than a file read would add, but the acceptance wording says "a WAV file played by your kernel".
9. **F4-13 uses a mock TPM** (see above). Please decide whether the chapter may be published before a swtpm rerun.
10. **Course forensic labs.** The course plan lists "Packets vanish at 5 % loss" (F4-10's forensic: a retransmission timer that is not re-armed, with a tshark capture) and "DMA to nowhere" (F4-08's forensic: an IOMMU fault report). Both are built as described. The plan's exam item "annotate a USB enumeration capture" can use F4-09's `capture.out`.

## QEMU behaviour found while building the labs

- **slirp ignores a repeated SYN** (F4-10): when its SYN-ACK is lost, QEMU's user-mode router does not answer the guest's retransmitted SYN and resends the SYN-ACK only after seconds, so the lab's connect timeout is 20 s. Observed in this build, not checked in libslirp's sources.
- **guestfwd with `cmd:` writes the command's stderr into QEMU's output** (F4-10); `run.sh` filters it from the kernel log.
- **e1000 receive overflow without induced loss** (F4-10): during the 100 MiB echo, MPC and RNBC both counted about 41,000 events and TCP recovered; 0 in the IOMMU run. Whether QEMU counts frames or delivery attempts was not established.
- **intel-hda ignores a byte write to SDnCTL's third byte** (F4-12): the stream number must be set with a 32-bit write (with a zero top byte so the status bits are not cleared).
- **hda-output's pin widget has no output amplifier** (F4-12): the driver sets gain only on widgets that report one.
- **qemu-xhci with intel-iommu** (F4-09): the DMAR fault when translation is enabled (decision 6).
- **SeaBIOS uses the devices before the kernel**: USB captures start with 14 firmware frames (CBW tag 0x3e7); the TPM log contains SeaBIOS's measurements; PCI interrupt line registers hold PIC-mode values.
- **QEMU's tpm-emulator backend sends command 0x181 before Startup** (probably its version probe).

## Open issues

- **Timing figures are emulated.** PIT, PM timer and audio clocks under TCG disagree by a few percent (F4-11: the PM timer measured 3.57 MHz in the final run and 3.68 MHz in an earlier one, against 3.58 nominal; F4-12: 8 periods in about 1.95 s against 2.003 s). The chapters say so and draw no conclusions from them.
- **The audio recording is 30–40 ms short at the end** (F4-12), varying per run; the chapter gives the likely cause as unverified.
- **F4-10's forensic double drop** at sequence 34311 comes from the loss model (a hash of sequence number and retransmission count, reset by each ACK), which the forensic key explains.

## Analogy proposals (F4: the school and its visitors)

These extend the faculty analogy. Please accept, reject or rename them.

- **A note pushed into the office's mail slot; the storeroom gatekeeper with a list of shelves per truck:** MSI; the IOMMU (F4-08).
- **The reception desk with badge numbers and self-description forms:** the xHCI host controller, slot IDs, descriptors and enumeration (F4-09).
- **The post room with numbered pages, receipts and a clock for unsigned pages:** TCP sequence numbers, ACKs and the retransmission timer (F4-10).
- **The caretaker's binder in shorthand; the night bell:** ACPI AML; the SCI (F4-11).
- **The announcement system: a note tray, a reply tray and a tape loop:** CORB, RIRB and the BDL (F4-12).
- **The porter's sealed tally and the open visitors' book:** PCRs and the event log (F4-13).

## Glossary

`glossary.json` has 67 entries, one for each glossary link in the chapters, except for seven terms that are already defined, under the same name, in courses sorted before or after DR302: End-of-interrupt (EOI), Doorbell register (DR301); IOMMU, Enumeration (USB) (HW204); IOAPIC (OS302); AML (ACPI Machine Language) (OS305); Measured boot (OS301). Those links resolve to the existing entries. DR302's entries would win over HW204's and the OS courses' (the merge keeps the first course in alphabetical order), so they were left out on purpose.

## Build check

`python3 university/build/build.py` reports no PROBLEM line that mentions DR302, F4-08 to F4-13, or any anchor these chapters link to (the remaining PROBLEM lines belong to other courses).
