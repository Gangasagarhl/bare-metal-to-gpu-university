# HW204 — Buses, I/O, interrupts and DMA: author notes

Chapters F1-40 to F1-48, L2–L3. Prerequisite course: HW202.
Labs are in `university/labs/F1-40` to `university/labs/F1-48`.
Each lab passes `university/labs/run_lab.sh university/labs/<ID>` with exit code 0. All nine were re-run at the end of this build, on 2026-10-09. After that re-run, the numbers quoted in the prose were checked against the outputs.

## Toolchain and local evidence (as recorded in the logs)

- **g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0.** Uses `-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined`. F1-41 also builds `poll.cc` and `driver_bug.cc` at `-O0` and `-O2` without sanitizers, because those runs show the optimiser's effect.
- **QEMU emulator version 8.2.2 (Debian 1:8.2.2+ds-0ubuntu1.18).** Uses `qemu-system-x86_64` (q35), `qemu-system-riscv64` (virt) and `qemu-system-aarch64` (virt, GICv3). Monitor commands are sent over QMP by `hmp.py` (`human-monitor-command`). A copy of `hmp.py` is in F1-41, F1-43, F1-44, F1-45 and F1-46. Only firmware runs in these machines; no guest OS is booted.
- **TShark (Wireshark) 4.2.2.** Decodes QEMU's USB pcap in F1-46.
- **lspci version 3.10.0.** Runs in F1-45 on the build machine.
- **Linux UAPI headers, linux-libc-dev 6.8.0-146.146.** These were opened and used as tier-4 local sources, and the listings include them: `linux/pci_regs.h` (F1-43, F1-44, F1-45), `linux/usb/ch9.h` (F1-46), `linux/serial_reg.h` (F1-41, F1-42, F1-47), `linux/i2c.h` and `linux/can.h` (F1-47).
- **The build machine.** It is a virtual machine. It has a real `/sys/bus/pci` (virtio devices, host bridge 8086:0d57), `/proc/interrupts` (IO-APIC, PCI-MSIX, LOC rows) and clocksource `tsc` (available: `tsc kvm-clock`).

## Listings run

**Nothing was run on real hardware.** Every run is a C++ model, a QEMU emulator run, or a read of the build machine's Linux files.

| Chapter | Run | Status |
|---|---|---|
| F1-40 | bus_decode | pass |
| F1-40 | bus_arbiter | pass |
| F1-40 | bus_overlap | forensic evidence, exit 0 (TIMER window 0x100 overlaps the UART; contention) |
| F1-41 | mmio_regs | pass |
| F1-41 | poll_O0 / poll_O2 | demo, exit 0 (at -O2 the plain wait loop compiles to `ret`) |
| F1-41 | driver_bug_O0 / driver_bug_O2 | forensic evidence, exit 0 (-O2 misses the device's write) |
| F1-41 | portio_asm | demo, exit 0 (disassembly of in/out) |
| F1-41 | portio | **expected fail by design: exit 139 (SIGSEGV, `in` refused in user mode)** |
| F1-41 | qemu_mtree | QEMU, exit 0 (filtered memory and I/O maps) |
| F1-42 | uart_poll | pass |
| F1-42 | uart_irq | pass |
| F1-42 | uart_field | forensic evidence, exit 0 ("The missing bytes", the course forensic lab) |
| F1-42 | irq_delta | host measurement, exit 0 (volatile counts) |
| F1-43 | pic_sim | pass |
| F1-43 | pic_noeoi | forensic evidence, exit 0 (missing EOI) |
| F1-43 | qemu_x86_pic, qemu_riscv_mtree, qemu_arm_mtree | QEMU, exit 0 |
| F1-43 | proc_interrupts | host read, exit 0 (volatile counts) |
| F1-44 | dma_copy, dma_coherence | pass |
| F1-44 | dma_forensic | forensic evidence, exit 0 (partial invalidate) |
| F1-44 | qemu_iommu | QEMU with `intel-iommu`, exit 0 (per-device `vtd-root` address spaces) |
| F1-45 | cfg_read | host read of the PCI config space, exit 0 (the output depends on the machine) |
| F1-45 | lspci, lspci_names | host tool, exit 0 |
| F1-45 | qemu_info_pci | QEMU, exit 0 (the course lab "QEMU monitor inspection of PCI") |
| F1-45 | bar_size | pass |
| F1-45 | bar_size_bug | forensic evidence, exit 0 (flag bits not masked) |
| F1-46 | qemu_info_usb, usb_capture, usb_bytes | QEMU and tshark, exit 0 |
| F1-46 | usb_desc | pass (decodes the real captured bytes) |
| F1-46 | usb_poll | forensic evidence, exit 0 (polled pedal) |
| F1-47 | i2c_read, uart_frame, spi_xfer, can_arb | pass |
| F1-47 | i2c_forensic | forensic evidence, exit 0 (8-bit address used as a 7-bit address) |
| F1-48 | timer_calc | pass |
| F1-48 | wrap | forensic evidence, exit 0 (wrap-around comparison) |
| F1-48 | host_clock | host measurement, exit 0 (volatile) |
| F1-48 | clocksource, timer_irqs | host read, exit 0 (timer_irqs is volatile) |

### Untested on hardware (each has an unverified box, and a safety box where it applies)

- **F1-42 lab, board part.** UART echo on a microcontroller, first polling, then with interrupts: the course lab.
- **F1-47 lab, Part B.** Logic-analyser I2C capture with sigrok/PulseView: the course lab. sigrok was not installed on the build machine.
- **F1-48 mini-project.** The course project: an interrupt-driven sensor reader on an MCU, with a logic-analyser trace.

### Volatile measurements (AH-23)

The prose does not quote these numbers. They change on every run:

- `irq_delta` (F1-42) and `proc_interrupts` (F1-43): interrupt counts.
- `timer_irqs` (F1-48): local timer interrupts per CPU.
- `host_clock` (F1-48): sleep durations.
- `driver_bug_O0` (F1-41): the waited time.
- The `info irq` counts in `qemu_x86_pic` (F1-43). They were identical across the re-runs in this build, but the prose names only their pattern: line 2 = line 8.
- The capture timestamps in `usb_capture` (F1-46).

### Ad-hoc runs, not kept as listings

- **F1-41, answer 6.** Removing `volatile` from `mmio_regs`'s `reg_read` at -O2 gave identical output.
- **F1-43, answer 6.** `pic_sim` with the timer raised at t = 6: the timer nests inside the DISK handler.
- **F1-47, answer 6.** `uart_frame` with a 3% long bit time decodes "Hi".

## Unverified boxes, per chapter

- **F1-40.** PCIe ordering rules (posted writes, reads, completions).
- **F1-41.** Memory types for device registers (x86 PAT/MTRR, Arm Device memory).
- **F1-42.**
  - The registers saved automatically on interrupt entry, the stack used, and the return instruction, per architecture.
  - The board step: no board, toolchain or terminal program was named or tested. This part also has a safety box.
- **F1-43.** The EOI procedures and spurious-interrupt rules for the 8259A, the local APIC/x2APIC, GICv3 and the PLIC.
- **F1-44.**
  - Which platforms are DMA-coherent (x86 PCIe, Arm/RISC-V SoCs, No Snoop).
  - Linux barrier semantics (`wmb`, `dma_wmb`, `writel`).
- **F1-45.**
  - The ECAM offset formula and the 0xCF8 address format. These were written from memory and checked only for consistency with QEMU's 256 MiB `pcie-mmcfg-mmio` window.
  - PCIe link speeds, widths, payload sizes and ordering. No numbers are given.
- **F1-46.**
  - USB speed names and rates beyond the 12 and 480 Mb/s that QEMU printed.
  - Why SET_ADDRESS is absent from the capture (the xHCI Address Device command).
  - The encoding of `bInterval`.
- **F1-47.**
  - All bus speeds, voltages, pull-up and termination values. None are given.
  - Part B: the logic-analyser steps. This part also has a safety box.
- **F1-48.**
  - Timer counting conventions (reload inclusive, up or down).
  - TSC, HPET, local APIC timer and SysTick details.
  - The course project on hardware.

The other claims marked from memory in the prose:

- **F1-46.** The guarantees column of the transfer-type table.
- **F1-47.** The CAN acknowledge slot and resynchronisation in the comparison table.

Both say so where they appear.

## All D sources are title only

*Superseded by the verification pass of 2026-10-10 (see "Verification pass" below): the sources that could be opened are now cited with edition and section in the chapters and in `university/_dossiers/F1-4x.dossier.html`; the rest are marked as not opened, with the reason.*

Every book, specification and datasheet entry (Dx) is marked "Title only — not opened during this build (dossier gate G1 open)". The Source Researcher must confirm editions and sections. The main ones:

- Patterson & Hennessy.
- Harris & Harris.
- The Intel SDM Vol. 3, and the 8259A, 82093AA and PC16550D datasheets.
- The PCI Local Bus and PCI Express Base specifications.
- *PCI Express System Architecture*.
- The Arm GIC and SMMU specifications, and the Armv7-M ARM.
- The RISC-V PLIC and ACLINT specifications.
- Intel VT-d.
- The xHCI specification, USB 2.0 and the HID class definition.
- *USB Complete*.
- NXP UM10204 (I2C), Bosch CAN 2.0 / ISO 11898-1, and the SPI Block Guide.
- The sigrok/PulseView documentation.
- *The Art of Electronics*, *Making Embedded Systems*, *Linux Device Drivers* and *Linux Kernel Development*.
- ISO C++.
- The Linux kernel documentation files named in the boxes: `dma-api.rst`, `dma-api-howto.rst` and `memory-barriers.txt`.

## Decisions for the owner

1. **Course kit.** No microcontroller board, I2C sensor or logic analyser is named anywhere. The three hardware parts are written so that they work with any board, and each is marked untested on hardware. Choosing a kit would let a Lab Engineer run them and replace the unverified boxes.
2. **Where the course labs live.** The labs from the course card are placed as follows:
   - UART echo, polling then interrupts: F1-42.
   - Logic-analyser I2C capture: F1-47, Part B.
   - QEMU monitor inspection of PCI: F1-45.
   - The forensic lab "The missing bytes": F1-42.
   - Exam task P, decoding a captured I2C transaction: F1-47, Worked example 1. It uses the text capture, because no real capture exists yet.
   - The course project: the F1-48 mini-project.
3. **Extra QEMU evidence beyond the course card.**
   - F1-43: x86 `info pic`/`info irq`, plus the RISC-V and Arm memory maps.
   - F1-44: `intel-iommu` memory tree.
   - F1-46: USB pcap capture.

   Please confirm that QEMU 8.2.2 and tshark are acceptable tools for the course.
4. **Glossary.**
   - `glossary.json` has 74 terms. "Bus" is left out on purpose: it is already defined (KID101), and F1-40 links to `#gl-bus`.
   - F1-41's "Port I/O" no longer carries the abbreviation "PIO", which F1-44 uses for "Programmed I/O".
   - "Enumeration" (PCI, F1-45) and "Enumeration (USB)" (F1-46) are separate entries. They could be merged.
5. **The F1-45 build-machine output depends on the machine.** `cfg_read` and `lspci` describe whatever machine runs the lab. The prose names the build machine's devices and says that the reader's will differ.

## Proposed analogy mappings (world F1, restaurant building)

These are new mappings, for the analogy registry:

- **F1-40.**
  - Bus: the service corridor and its lift.
  - Address bus: the room-and-shelf label on a trolley.
  - Data bus: the trolley's load.
  - Control signals: the deliver/collect stamp and the "now" bell.
  - Address decoder: the doorkeeper.
  - Memory map: the floor plan.
  - Arbitration: deciding which trolley enters the corridor first.
  - Bus contention: two doors answering one label.
- **F1-41.**
  - Device register / MMIO: hatches in the corridor wall that look like pantry shelves.
  - Port I/O: the intercom panel with its own button numbers.
  - W1C: the "seen it" button.
  - RMW: moving only your own jar on a shared shelf.
  - volatile: "look again each time".
- **F1-42.**
  - Interrupt: the doorbell.
  - Polling: walking to the back door.
  - IRQ: pressing the bell button.
  - Vector: the bell's room number on the kitchen panel.
  - Overrun: the box knocked off the step.
  - Ring buffer: the shelf beside the door.
  - Latency: finishing the stir first.
- **F1-43.**
  - Interrupt controller: the panel of bell lights by the pass, with silence switches and urgency numbers.
  - EOI: the "done" button.
  - I/O APIC and local APIC: a panel per floor and a pager per chef.
  - MSI: a note through a slot instead of a bell wire.
- **F1-44.**
  - DMA: the delivery truck.
  - Descriptor: the delivery note.
  - Descriptor ring: the clipboard of notes.
  - Cache: the chef's worktable.
  - IOMMU: the gatekeeper at the loading bay.
  - Bounce buffer: the trolley at the gate.
- **F1-45.**
  - PCIe: a food hall with a private corridor per stall.
  - Configuration space: the standard card at the stall door.
  - BAR: the space where the manager writes the room numbers.
  - Enumeration: the manager's morning walk.
  - Switch or bridge: a corridor junction with its own card.
- **F1-46.**
  - USB host: the waiter on a fixed round.
  - Gadgets never speak first.
  - Descriptor: the gadget's ID card.
  - Hub: the side table.
- **F1-47.**
  - I2C: the shared speaking tube where you call a name.
  - SPI: private string telephones with a cord per scale.
  - UART: the line to the bar at an agreed speed.
  - CAN: the van's shared wire where the most urgent message wins.
  - Logic analyser: a recorder on every tube.
  - Open-drain: a rope anyone can pull down, held up by a spring.
- **F1-48.**
  - Oscillator: the pendulum.
  - PLL and divider: gears.
  - Hardware timer: the oven timer.
  - Watchdog: a dead man's switch.
  - Wrap-around: the clock face going from 12 back to 1.

Each chapter has a "Where the analogy breaks" section. These are the points that matter most:

- Edge versus level requests (F1-43).
- Interrupt endpoints that are polled (F1-46).
- Wrap-around values (F1-48).

## Owner rulings applied

Applied on 2026-10-10 (verification pass) to the five open decisions above:

1. **Course kit** → ruling D1 (`university/build/KIT.md`). The three hardware parts now name the kit items: Raspberry Pi Pico 2 (RP2350) board, alternative ST NUCLEO-F446RE; Raspberry Pi Debug Probe as SWD and 3.3 V USB-to-UART bridge; Adafruit LSM6DSOX breakout as the I2C sensor; a sigrok-supported Cypress FX2 logic analyser running fx2lafw (alternative Digilent Analog Discovery 3 with WaveForms); PulseView. The sentences "the owner has not chosen the course kit" (F1-47, F1-48) are gone. Ruling C3: every hardware step stays marked "untested on hardware" and now says what is needed to test it (F1-42 step 6, F1-47 Part B, F1-48 project). Ruling C4: these three chapters are "internally checked · hardware steps untested".
2. **Where the course labs live** → decided by verifier: the placement stands as listed (UART echo and "The missing bytes" in F1-42; I2C capture in F1-47 Part B; QEMU PCI inspection in F1-45; exam task P on the text capture in F1-47; the project in F1-48). Ruling A2 applies to the text capture and the C++ bus/UART/PIC/DMA models: the chapters say they are course-own models and name the real items they stand for.
3. **Extra QEMU evidence; QEMU 8.2.2 and tshark** → decided by verifier under ruling A5 (recorded runs stay) and KIT.md's QEMU row (11.1.2 is current, runs used 8.2.2 and say so): the QEMU 8.2.2 and TShark 4.2.2 evidence stays, labelled with its version and as evidence of QEMU's model only. Ruling C1 (open every source) could not be met for the QEMU documentation pages (lookup budget); the chapters say the help text is the installed program's own output.
4. **Glossary** → ruling A7: "Bus" stays defined once (KID101) and linked from F1-40; "Enumeration" (PCI) and "Enumeration (USB)" stay separate because their meanings differ (decided by verifier); "Port I/O" keeps no "PIO" abbreviation. `glossary.json` source strings now record the verification status of each source instead of "pending verification".
5. **F1-45 build-machine output varies** → decided by verifier: kept, with the prose saying the reader's machine will differ; ruling A5 keeps the recorded run.

Rulings A8 (committed test keys) and A10 (licence placeholder) do not apply: the HW204 lab folders contain no keys and no `LicenseRef-Uni-Lab` lines.

## Verification pass

Date: 2026-10-10. Checker: Fact-Checker agent. Dossiers: `university/_dossiers/F1-40..F1-48.dossier.html`; QA records: `university/qa/F1-40..F1-48.json`.

**Web access.** The shared web-fetch budget was exhausted for most of the pass; roughly one fetch in ten succeeded. Everything that could not be opened is marked "not opened" with the reason in the chapter's Sources and in the dossier's "Not found" list.

**Opened (full documents or sections):** RISC-V PLIC Specification 1.0.0 and ACLINT 1.0-rc4 (F1-43); Linux "Dynamic DMA mapping Guide" and LDD3 chapter 15 (F1-44); LDD3 chapter 10 (F1-42); Linux `include/linux/pci-ecam.h` (F1-45, new source L2); HID class definition 1.11 (F1-46); NXP UM10204 Rev. 7.0 and the PulseView User Manual 0.4.2 (F1-47); C++ working draft sections [intro.abstract] (F1-41), [basic.fundamental], [time.clock.steady], [thread.thread.this] (F1-48); GCC manual "Extended Asm" and "Machine Constraints" (F1-41).
**Opened (catalogue or landing pages only):** Harris & Harris RISC-V edition (2021), Patterson & Hennessy RISC-V edition 2nd ed. (2020), CS:APP 3rd ed. (2015), The Art of Electronics 3rd ed. (2015), ISO 11898-1:2015 (ruling C2), the Intel SDM download page (version 093).
**Not opened:** PC16550D, 8259A, 82093AA datasheets; PCI Express Base and PCI Local Bus specifications (PCI-SIG refuses automated access; cited by title, C2); Arm GIC, SMMU and Armv7-M/v8-M manuals (Arm site returned no content); Intel SDM chapters; VT-d; xHCI; USB 2.0; Bosch CAN 2.0; SPI Block Guide; QEMU documentation pages; Linux `proc.rst`, `memory-barriers.txt`, `arch/x86/pci/direct.c`; [intro.progress], [thread.req.timing]; *USB Complete*, *PCI Express System Architecture*, *Linux Kernel Development*, *Making Embedded Systems*.

**Corrected or made precise:** F1-43 PLIC claim/complete (now in the text with the 0-if-none rule; EOI box narrowed to 8259A, local APIC, GICv3); F1-44 barrier sentence (wmb() as the Linux guide shows; read barrier where the architecture needs one) and D9's title; F1-45 ECAM formula (out of the box, stated as what Linux computes, tagged L2); F1-46 SET_PROTOCOL/SET_IDLE codes and HID sections; F1-47 bus-clear wording (nine clocks then STOP), I2C speeds from UM10204 §5, CAN facts from the ISO catalogue abstract, PulseView steps tagged; F1-48 steady_clock property tagged. No lab code or expected output needed a change.

**Left unverified (boxes, with reasons):** F1-40 PCIe/AMBA ordering; F1-41 MMIO memory types, and the forward-progress sentence (Sources note); F1-42 registers saved on entry, board step; F1-43 EOI for 8259A/local APIC/GICv3; F1-44 which platforms are DMA-coherent, dma_wmb/writel ordering; F1-45 0xCF8 format, link speeds; F1-46 USB speeds and frame timing, xHCI Address Device, bInterval, bMaxPower units, transfer guarantees; F1-47 SPI modes, UART sampling/divisor, CAN ack/stuffing/resync, Part B; F1-48 reload conventions, TSC/HPET/APIC/SysTick, project.

**Labs.** All nine lab folders re-run with `run_lab.sh` on 2026-10-10: every program exited 0. Differences were only dates, QEMU FlatView numbers, firmware interrupt counts, USB capture timestamps and sleep timings; the recorded files were restored with `git checkout` (ruling A5). Hardware steps (F1-42 step 6, F1-47 Part B, F1-48 project) remain untested on hardware.

**Diagrams, editing, accessibility.** All 18 SVG figures have role=img, title, desc and caption, use only `sv-*` classes, and carry meaning by labels or dash patterns as well as colour; captions state arrow meaning and "not to scale" where needed. No phrase from guide 13.8 is used; sentence length averages 17–20 words (L2–L3). All tables have header cells; heading order is h2/h3; every claim tag resolves to a Sources entry. Nothing was trimmed (A1).
