# OS305 — Device firmware and real-time operating systems: author notes

Chapters F3-36 to F3-42. Level L3, 4 credits. Prerequisites: HW204, SP201; HW303 in parallel.
Labs are in `university/labs/F3-36` to `university/labs/F3-42`.

Each lab passes `university/labs/run_lab.sh university/labs/<ID>` with exit code 0. All seven were re-run at the end of this build, on 2026-10-09, and the numbers quoted in the prose were checked against the outputs after the re-run.

Two outputs change from run to run, and the prose says so:
- the tick count in F3-37 `app.out`;
- the number of hung runs in F3-37's race experiment (`race.out`: 40 runs; batches of 20 hung between 0 and 8 times; the last re-run hung 8 of 40).

Every fragment passes the local checks:
- html.parser balance;
- ids prefixed with the chapter id;
- no URLs;
- no `<script>`;
- section order (21 template sections plus answers);
- every source reference resolves and every source is cited;
- every `data-src` and `data-run` file exists;
- every glossary link resolves (OS305 terms are in `glossary.json`; a few links go to HW204/HW205/HW303 terms).

`python3 university/build/build.py` reports no PROBLEM line for F3-36 to F3-42. The remaining PROBLEM lines name other courses' glossary links.

## Toolchain and local evidence (as recorded in the logs)

- **Ubuntu clang 18.1.3 and LLD 18.1.3.**
  - Target: `--target=thumbv7m-none-eabi -mcpu=cortex-m3`, with `-std=c++20 -ffreestanding -fno-exceptions -fno-rtti -nostdlib -Os -g -Wall -Wextra -Wpedantic -Werror`.
  - Our own linker scripts. Firmware sources are `.cc` files, built by each lab's `run.sh`, so that `run_lab.sh` does not build them as host programs.
- **g++ 13.3.0.** Builds the host models with `-fsanitize=address,undefined`: `i2c_sim`, `usbdev`, `forensic` (F3-38), `rta`, `bootsim`, `fleet`, `mkimage`, `ec_sim`, `forensic` (F3-42), `bmc_sim`. F3-41 links OpenSSL 3.0.13 (`-lcrypto`) for Ed25519.
- **QEMU 8.2.2, `mps2-an385` (Cortex-M3).**
  - Semihosting is used for exit codes.
  - Its emulated tmp105 sensor is on the board's I2C (SBCon) bus, used in F3-37 and F3-40.
  - `-icount shift=0,sleep=off` gives repeatable timing in F3-39 and F3-40.
  - The generic loader device places the slot images in F3-41.
  - `-trace` and the monitor (`info mtree`, `qom-set`) are used where noted.
- **LLDB 18.** Connects to QEMU's GDB stub (F3-36). The container's GDB is x86-only.
- **Other tools.** `sha256sum` cross-checks F3-41's SHA-256. The Linux UAPI header `linux/ipmi_msgdefs.h` supplies F3-42's verified IPMI constants.
- **Not available in the container, so nothing about them was compiled or run:** Zephyr, FreeRTOS, MCUboot, TinyUSB, Chromium EC, OpenBMC, a USB host/device emulation path, any real board or probe.

## Listings run

**Nothing was run on real hardware.** Every run is one of:
- a host C++ model;
- a QEMU emulator run;
- a debugger session against QEMU.

| Chapter | Run | Status |
|---|---|---|
| F3-36 | mtree, build, size, symbols (os305.ld, startup.cc, board.h, main.cc) | pass (text 1944, data 4, bss 8) |
| F3-36 | boot, trace | QEMU, exit 0 through semihosting. Untested on hardware |
| F3-36 | session (session.lldb) | LLDB on QEMU's GDB stub: breakpoint in Reset_Handler hit, exit 0. Untested with a probe |
| F3-36 | forensic_boot, forensic_trace | **expected fail by design: exit 1** (constructors never run) |
| F3-36 | forensic_elf | forensic evidence, exit 0 |
| F3-37 | irqmap, monitor, app, i2c_trace (i2c_bitbang.h, irqmap.cc, app.cc) | QEMU, exit 0 (sensor 23.50, then 23.75 after `c`). Untested on hardware |
| F3-37 | i2c_sim (host model of the bit-banged bus) | pass |
| F3-37 | race (app_race.cc, 40 runs) | exit 0; some runs hang by design (the lost-interrupt race) |
| F3-37 | size | pass (text 2212, bss 32) |
| F3-37 | forensic_diff | **expected exit 1** (`diff`: the files differ) |
| F3-37 | forensic_uart | **expected fail by design: exit 124** (interrupt storm, 2 s limit) |
| F3-37 | forensic_nvic | forensic evidence, exit 0 |
| F3-38 | usbdev (usb_stack.h, usbdev.cpp) | host model of enumeration, pass. No USB host, no TinyUSB, untested on hardware |
| F3-38 | forensic (wTotalLength 0x12) | forensic evidence, exit 0 |
| F3-39 | app, repeat (urtos.h, urtos.cc, app.cc) | QEMU with -icount, exit 0. The filter measured 8.8 ticks against an RTA prediction of 8.0. Untested on hardware |
| F3-39 | rta (rta.cpp) | pass |
| F3-39 | size | pass (text 3784, data 84, bss 6064) |
| F3-39 | forensic (priority swap) | exit 0; the sensor misses 8 of 12 deadlines, as designed |
| F3-40 | app (inheritance on), forensic (inheritance off), size | QEMU with -icount, exit 0. Worst case 0.80 ticks with inheritance on; 3.80 ticks and 4 misses with it off. Untested on hardware and without Zephyr or FreeRTOS |
| F3-41 | bootsim (ota.h, bootsim.cc) | pass: "H4 checks PASS". A/B 0 of 29 cut points brick the device; single slot 27 of 27 brick it |
| F3-41 | fleet_single, fleet_ab (fleet.cc) | forensic evidence, exit 0 (496 bricked, against 0 bricked) |
| F3-41 | images (mkimage.cc, sha256.h, image.h, boot.cc, app.cc) | pass; SHA-256 equals `sha256sum` |
| F3-41 | boot_ab, boot_damaged | QEMU, exit 0. Untested on hardware |
| F3-41 | boot_empty | **expected exit 1 by design** (recovery mode, no valid image) |
| F3-42 | ec_sim (ec_model.h, ec_sim.cpp), bmc_sim | pass (65 %; 0 wrong in 1,000 reads) |
| F3-42 | forensic | forensic evidence, exit 0 (119 %, 174 %, 94 %, 64 %) |
| F3-42 | machines | QEMU machine list, exit 0. No BMC image booted |

## Track H milestones

| Milestone | Status in this build |
|---|---|
| H1 (F3-36) | Met in emulation: UART banner, a breakpoint in the reset handler (LLDB instead of GDB, QEMU stub instead of a probe), size and RAM use reported. **Untested on hardware.** |
| H2 (F3-37) | Met in emulation, on QEMU's tmp105 model: the reading is correct against the value set from the monitor, and there is an I2C trace from QEMU's own model. **Not met:** a logic-analyser trace and timing on a real bus. |
| H3 (F3-38) | Host model only: descriptors and enumeration, including the forensic wTotalLength bug. **Not met:** an OS enumerating a real device, a TinyUSB build, a USB capture. |
| H4 (F3-39 to F3-41) | Both acceptance items met in the host model: a wrong signature is refused, and a simulated power loss never bricks the device. The on-target boot loader is hash-only, running in QEMU. The two-task RTOS is our own uRTOS. **Not met:** Zephyr or FreeRTOS, MCUboot, a signature check on the target, real flash. |
| H5 (F3-42) | The hop map is built on a model. **Not met:** naming real Chromium EC files and ACPI sections for each hop, because the sources were not available; the optional EC on the H2 board is not built either. |

Exam P (add a sensor task with a measured deadline) is prepared in F3-39 and F3-40. The course project H4 is F3-41's mini-project.

## Unverified boxes, per chapter

Each chapter has two boxes: one for facts recalled from memory, and one "untested on hardware" box with the owner's decisions.

- **F3-36.**
  - The reset behaviour, the Thumb bit, the vector-table layout, semihosting operation numbers, and the mps2-an385 memory map beyond what QEMU's `info mtree` showed.
  - Untested on a board or a probe.
- **F3-37.**
  - NVIC registers other than ISER0 and ICPR0, the stacking order and EXC_RETURN, and I2C timing from UM10204.
  - The tmp105 register details beyond what QEMU returned.
  - Untested: sensor, board, analyser.
- **F3-38.**
  - The boot-keyboard report descriptor bytes, usage values, descriptor field details, the ZLP rule, the minimum EP0 packet size of 8, and TinyUSB API names.
  - Untested: board, stack route (TinyUSB or own stack), analyser.
- **F3-39.**
  - The exception frame, PSP/MSP, EXC_RETURN 0xFFFFFFFD, PendSV and ICSR bits, and the RTA references.
  - Untested on hardware: all timing is QEMU instruction-counted virtual time.
- **F3-40.**
  - Table 1, the uRTOS / FreeRTOS / Zephyr API mapping: no name in it was compiled.
  - Devicetree and Kconfig details.
  - Untested: Zephyr, FreeRTOS, hardware.
- **F3-41.**
  - MCUboot: slot names, upgrade strategies, image header and TLV formats, imgtool, security counter, and the Zephyr integration.
  - The VTOR alignment rule and the barriers after writing it; FIPS 180-4 and RFC 8032 as definitions.
  - Untested on hardware and with MCUboot.
- **F3-42.**
  - ACPI EC: the command codes 0x80/0x81/0x84, the OBF/IBF/SCI_EVT bit positions, and the `_Qxx` methods.
  - Smart Battery registers.
  - IPMI: chassis netfn and command, Get Sensor Reading 0x2D, completion code 0xCB, KCS/SSIF/LAN details, and the length of a real Get Device ID response.
  - Redfish, OpenBMC and Chromium EC structure.
  - Windows and Linux driver names.
  - Untested on hardware.

## Decisions for the owner

1. **Lab board** (H1–H4). It needs:
   - a Cortex-M part with public reference manuals;
   - a USB device port (H3);
   - a documented flash layout and recovery path (H4).

   No board is named in the chapters.
2. **Debug probe and host software:** OpenOCD or pyOCD, and GDB. The container's GDB is x86-only, so the chapters use LLDB.
3. **Sensor for H2 and exam P:** an I2C temperature sensor with a datasheet. The emulated part is QEMU's tmp105 model.
4. **Logic analyser** for H2 and H3 captures.
5. **RTOS for H4:** Zephyr or FreeRTOS, and the version. Table 1 in F3-40 must then be checked against it.
6. **USB route for H3:** TinyUSB first, then our own stack, as the curriculum suggests. Also needed: the USB vendor and product IDs to use in labs. The model uses placeholder values; a policy is needed for anything plugged into a real PC.
7. **Boot loader for H4:** MCUboot, which strategy and which version, or the course's own loader. Also decide the signature algorithm, how private keys are handled (the teaching keys are derived from fixed seeds), and whether a target-side signature check becomes a required lab.
8. **EC and BMC (H5):** which Chromium EC version learners read, whether the course supplies a BMC firmware image for QEMU, and whether the optional "tiny EC on the H2 board" becomes required. F3-42's mini-project moves it to the emulator.
9. **Sources:** all D-sources are "title only, not opened" (dossier gate G1 open). The Source Researcher must confirm editions and sections.

## Analogy proposals (F3, the school)

- **F3-36.** The caretaker who opens the building in the morning (firmware and start-up code); the building plan in the caretaker's office (BSP).
- **F3-37.** The doorbell (interrupt) and the school office that decides which bell is answered first (interrupt controller).
  - A bell that is never acknowledged keeps ringing (interrupt storm).
- **F3-38.** A visiting speaker who must hand the office a CV before being given a room (enumeration and descriptors); the interpreter of guide 8 (the driver).
- **F3-39.** The timetable (scheduler); groups needing the one sports hall (tasks); the rule printed on the hall door (priority).
- **F3-40.** The one key to the science lab (mutex).
  - The exam group waiting through an assembly (priority inversion).
  - Lending the "urgent" badge (priority inheritance).
- **F3-41.** Two classrooms and a trial day (A/B slots with test and confirm).
  - The signed appointment letter (signature).
  - Gate guards checking badges (chain of trust, shared with F11).
- **F3-42.** The caretaker's hatch with an in-tray and an out-tray (EC interface, IBF/OBF).
  - The night watchman with his own keys and phone line (BMC).

Where each analogy breaks is stated in each chapter's "Where the analogy breaks" section.
