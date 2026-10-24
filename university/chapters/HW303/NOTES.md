# HW303 — Microcontrollers and embedded hardware: author notes

Chapters F1-73 to F1-79. Level L3, 3 credits. Prerequisites: HW204, SP201.
Labs are in `university/labs/F1-73` to `university/labs/F1-79`.

Each lab passes `university/labs/run_lab.sh university/labs/<ID>` with exit code 0. All seven were re-run at the end of this build, on 2026-10-09. After the re-run, the numbers quoted in the prose were checked against the outputs.

One exception: the SysTick current value in F1-77 changes from run to run. The prose says so.

Every fragment passes the local checks:
- html.parser balance;
- ids prefixed with the chapter id;
- no URLs;
- no `<script>`;
- section order;
- source references resolve;
- every `data-src` and `data-run` file exists.

`python3 university/build/build.py` reports no PROBLEM line for F1-73 to F1-79.

## Toolchain and local evidence (as recorded in the logs)

- **Ubuntu clang version 18.1.3 and Ubuntu LLD 18.1.3.**
  - Target: `--target=thumbv7m-none-eabi -mcpu=cortex-m3`, with `-std=c++20 -ffreestanding -fno-exceptions -fno-rtti -nostdlib -Wall -Wextra -Wpedantic -Werror`.
  - Optimisation is `-Os`, except F1-77, which uses `-O0` for debugging.
  - Linking uses our own linker scripts with `--gc-sections` and `-Map`.
  - The thumb target has no C++ standard library headers here, so firmware uses `<stdint.h>`.
  - Firmware sources use the `.cc` extension, so that `run_lab.sh` does not try to build them as host programs. Each lab's `run.sh` builds them.
- **riscv64-linux-gnu-g++ 13.3.0.** Used for the RISC-V comparison in F1-73: `-march=rv64imac -mabi=lp64 -mcmodel=medany -fno-pic -no-pie -nostartfiles -static`.
- **g++ 13.3.0.** Builds the host models with `-fsanitize=address,undefined`:
  - clocktree, pwm, tap, umcu_tim, netcheck.
- **QEMU emulator version 8.2.2.**
  - `mps2-an385` (Cortex-M3): used in F1-73, F1-76 and F1-77.
  - `stm32vldiscovery` (STM32F100 model): used in F1-74 and F1-75.
  - RISC-V `virt`: used in F1-73.
  - Monitor `info mtree -f`, `-d unimp`, and `-trace` (CMSDK timer, FPGAIO).
- **lldb 18.1.3.** Connects to QEMU's GDB stub over a Unix socket with `process connect --plugin gdb-remote`.
- **gdb.** The container's GDB is x86-only and cannot debug Arm, so it was not used.
- **Output scrubbing.** Local paths, process numbers and socket paths are removed from the outputs by the lab scripts.

## Listings run

**Nothing was run on real hardware.** Every run is one of:
- a host C++ model;
- a QEMU emulator run;
- a debugger session against QEMU.

| Chapter | Run | Status |
|---|---|---|
| F1-73 | build, size, symbols, vectors (startup.cc, main.cc, mps2_an385.ld) | pass (text 843, data 4, bss 4) |
| F1-73 | machines, mtree | QEMU, exit 0 |
| F1-73 | boot | QEMU, exit 124 by design: 3 s limit, firmware loops forever after printing the banner. Untested on hardware |
| F1-73 | rv_boot (RISC-V virt) | QEMU, exit 0 through the test device. Untested on hardware |
| F1-73 | forensic_boot | **expected fail by design: exit 134** (QEMU lockup). An empty `for(;;){}` was removed by the compiler and execution fell into the next function |
| F1-73 | forensic_symbols, forensic_disasm, good_disasm | forensic evidence, exit 0 |
| F1-74 | mtree, build, map, boot | pass. The boot is exit 124 by design (3 s limit), on stm32vldiscovery (text 1044) |
| F1-74 | forensic_build, forensic_map | forensic evidence, exit 0 (`.isr_vector` removed by `--gc-sections`; the course forensic "Nothing happens after reset") |
| F1-74 | forensic_boot | **expected fail by design: exit 134** (lockup) |
| F1-74 | forensic_reset | LLDB at reset: sp=0x466fb580, pc=0x0004f240 (garbage vectors). Exit 0 |
| F1-75 | clocktree | pass (U-MCU-1 model) |
| F1-75 | forensic | forensic evidence, exit 0 (UART divisor computed for the wrong clock: +503.86 %) |
| F1-75 | probe | QEMU, exit 124 by design. RCC/GPIOC writes are logged as unimplemented by `-d unimp`. Untested on hardware |
| F1-76 | pwm | pass |
| F1-76 | forensic | forensic evidence, exit 0 (off-by-one PSC/ARR: 985.316 Hz, 99.90 %) |
| F1-76 | blink | QEMU, exit 124 by design. "blink: 101010" plus software-PWM rows. Untested on hardware |
| F1-76 | trace, size | exit 0 (67 LED writes, 85 flag clears; text 784) |
| F1-77 | tap | pass (JTAG TAP model; invented device) |
| F1-77 | app_run | QEMU, exit 124 by design ("3 ticks seen") |
| F1-77 | session | LLDB session, exit 0. **Breakpoint in Reset_Handler hit (H1).** Also: SysTick handler, xPSR, watchpoint. Untested on hardware; LLDB instead of GDB |
| F1-77 | forensic_nm, forensic_session, forensic_uart | forensic evidence, exit 0 (misspelt `SysTick_handler` → weak default handler) |
| F1-78 | umcu_tim | pass (reference solution, PWM, and the worked example as part C) |
| F1-78 | forensic | forensic evidence, exit 0 (no UG, so the buffered PSC is not loaded; SR cleared with 0) |
| F1-79 | netcheck | pass (U-BOARD-1 rev A, 0 problems) |
| F1-79 | forensic | forensic evidence, exit 0 (rev B: 4 problems) |

Two answers are backed only by scratch runs, not by lab logs. The prose says so in both places.
- F1-78, Check-yourself question 7: the PSC change without UG gives intervals of 250, 125 and 125 ms.
- F1-79, worked example: R1 = 220 Ω gives 5.9 mA.

### Milestone H1 acceptance (curriculum 17.2), status in this build

Acceptance test, verbatim: "The UART prints a banner; a breakpoint in the reset handler is hit in GDB; the image size and RAM use are reported and fit the chip."

- **Banner.** QEMU mps2-an385 printed it (F1-73 `boot.out`).
- **Breakpoint in the reset handler.** It was hit with **LLDB 18**, not GDB, through QEMU's GDB stub, not a probe (F1-77 `session.out`).
- **Image size and RAM use.** Reported by `llvm-size` and the map file against QEMU's STM32F100 model sizes (F1-74).
- **Overall status: met in emulation only. Untested on hardware.**

## Unverified boxes (per chapter: what to check, and where) — as written by the authoring build

This list is historical: the verification pass of 2026-10-25 resolved most items. The current state is in "Verification pass" at the end of this file.

- **F1-73**
  - (1) How the RISC-V `virt` machine reaches 0x80000000 with `-bios none`. Check: QEMU RISC-V virt documentation; the RISC-V Privileged spec, reset section.
  - (2) The C++ forward-progress rule for empty loops. Check: ISO/IEC 14882, forward-progress section.
  - (3) The CMSDK APB UART register layout. Check: Arm CMSDK TRM, APB UART; the MPS2 AN385 application note.
  - (4) Untested-on-hardware box.
- **F1-74**
  - (1) STM32F100 boot-mode selection and memory sizes per part. Check: the STM32F100 datasheet and reference manual.
  - (2) Flash wait states, prefetch and programming. Check: the reference manual's flash chapter and the flash programming manual.
  - (3) The STM32F1 USART register offsets and bits. QEMU models this USART with an STM32F2xx-style model. Check: the STM32F100 reference manual, USART chapter.
  - (4) Untested-on-hardware box.
- **F1-75**
  - (1) STM32F1 RCC APB2ENR offset and bit, and the GPIOC CRH/ODR offsets and values. Check: the STM32F100 reference manual, RCC and GPIO chapters.
  - (2) U-MCU-1 values are teaching values only. Check: the lab chip's datasheet and its RCC chapter.
  - (3) Untested-on-hardware box.
- **F1-76**
  - (1) The CMSDK APB timer register layout, MPS2 FPGAIO LED register and timer clock. Check: Arm CMSDK TRM, APB timer; the MPS2 AN385 application note.
  - (2) Untested-on-hardware box: GPIO/timer chapters, schematic, logic analyser.
- **F1-77**
  - (1) Each of these needs checking against its own document:

    | Item | Document to check |
    |---|---|
    | JTAG TAP table, IDCODE and BYPASS rules | IEEE 1149.1 |
    | SWD packet format and DP/AP structure | Arm Debug Interface Architecture Specification (ADIv5/ADIv6) |
    | EXC_RETURN, the xPSR exception number, SysTick registers, FPB/DWT | Armv7-M ARM |
    | Number of Cortex-M3 hardware breakpoints | Cortex-M3 TRM |

  - (2) Untested-on-hardware box: no probe, board or Arm GDB.
- **F1-78**
  - (1) The general description of real vendor document sets, CMSIS-SVD and device headers. Check: the chosen lab chip's documentation set, timer chapter and errata.
  - (2) Untested-on-hardware box: U-MCU-1 is fictional.
- **F1-79**
  - (1) Real datasheet contents (group/total current limits, tolerant pins, conditions, pull ranges, unused pins) and schematic-tool features. Check: the chosen chip's datasheet and reference manual GPIO chapter, and the board schematic and user guide.
  - (2) Untested-on-hardware box: U-BOARD-1 is fictional, and the currents are ideal-pin estimates.

In the authoring build all book and specification sources were listed "title only — not opened". The verification pass of 2026-10-25 opened the specifications and rewrote every Sources list (see "Verification pass").

## Fictional teaching parts (labelled in every chapter that uses them)

- **U-MCU-1.** A teaching microcontroller.
  - Its clock tree is defined by `F1-75/clocktree.cpp`.
  - Its PWM timer rules are defined by `F1-76/pwm.cpp`.
  - Its TIM2 reference-manual excerpt is `F1-78/umcu1_rm_tim2.txt`, implemented by `umcu_tim.cpp`.
  - Its "datasheet" pin limit of 8 mA is set in the F1-79 netlists.
- **U-BOARD-1.** A teaching board, defined by `F1-79/netcheck.in` (rev A) and `rev_b.net` (rev B, the forensic file).
- **The F1-77 JTAG device.** Its IDCODE 0x0EDC0C01 and instruction codes are invented.

## Analogy proposals (F1 restaurant world). Approved under owner ruling A3.

These are proposed extensions of the F1 analogy world. They are used in the chapters but are not yet in the guide's analogy table.

| Concept | Proposed mapping | First used |
|---|---|---|
| Microcontroller | the take-away kitchen in one room | F1-73 |
| Flash (program memory) | the laminated recipe book screwed to the wall | F1-73, F1-74 |
| SRAM | the counter top | F1-73, F1-74 |
| Peripherals | the appliances (kitchen timer, hot plate, order window) | F1-73, F1-75 |
| Vector table | the first page of the recipe book | F1-73 |
| Reset handler / start-up code | the opening routine of the stand | F1-73 |
| Clock tree | the kitchen's rhythm (the beat everyone works to) | F1-75 |
| Clock gate | the wall switch of an appliance | F1-75 |
| Timer / PWM | the kitchen timer; flicking the lamp to dim it | F1-76 |
| Debug probe / halt | the inspector's pause button and hatch | F1-77 |
| Reference manual / datasheet / errata | the appliance's service manual / leaflet / "known issues" page | F1-78 |
| Schematic / datasheet limits | the stand's wiring plan / the rating plates | F1-79 |

## Decisions for the owner (answered: see "Owner rulings applied")

1. **The real lab microcontroller and board.**
   - Choose them, and supply or name the datasheet, reference manual, errata sheet, board schematic and user guide, with revisions.
   - Every "untested on hardware" box and the board-support-note project depend on this.
   - The emulated stand-ins are `mps2-an385` (Cortex-M3) and QEMU's STM32F100 model.
2. **The GDB requirement.**
   - The course card and H1 say "GDB". This build used LLDB because no Arm-capable GDB was installed.
   - Either install `gdb-multiarch` in the build container so the session can be re-run with GDB, or accept LLDB as equivalent for the emulated acceptance.
3. **The debug probe.**
   - Choose the probe and probe software (GDB server) for the lab board.
   - Nothing about JTAG/SWD signalling was exercised.
4. **QEMU limits.**
   - The `mps2-an385` CMSDK GPIO is not modelled, so the LED is the FPGAIO register.
   - `stm32vldiscovery` RCC, GPIO and timers are not modelled (logged as unimplemented).
   - Confirm that the emulation-plus-model approach is acceptable until hardware exists.
5. **Approve or change the analogy proposals** above.
6. **Approve the fictional teaching parts** U-MCU-1 and U-BOARD-1 as course conventions. They are also used by exam P (F1-78 excerpt) and the course project (F1-79).
7. **Unverified register layouts** need the Source Researcher (dossier gate G1):
   - the CMSDK UART and timer;
   - the STM32F1 USART, RCC and GPIO;
   - SysTick;
   - the JTAG TAP table and SWD format.

## Owner rulings applied

1. **Real lab microcontroller and board** — ruling D1 (reference kit in `university/build/KIT.md`): the Raspberry Pi Pico 2 (RP2350) or the ST NUCLEO-F446RE. Every "untested on hardware" box now names these boards and says what is needed to test the step (ruling C3). The RP2350 datasheet is now a cited tier-1 source in all seven chapters. ST's RM0041/STM32F446 documents could not be opened (server refused), so ST-specific behaviour stays in unverified boxes.
2. **GDB requirement** — no ruling covers it; decided by verifier: LLDB 18 through QEMU's GDB stub is accepted as the emulated evidence for the H1 breakpoint test (same GDB remote serial protocol; LLDB's "GDB to LLDB command map" lists the equivalent commands). The F1-77 note keeps saying that the GDB route was not run; when `gdb-multiarch` (or an Arm GDB) is installed, re-run the F1-77 session with GDB. The hardware run stays "untested on hardware" (C3); the catalogue shows "internally checked · hardware steps untested" (C4).
3. **Debug probe** — ruling D1: the Raspberry Pi Debug Probe (CMSIS-DAP, SWD only) for the Pico 2, or the NUCLEO-F446RE's on-board ST-LINK. JTAG stays emulated/theoretical (KIT.md note). Probe software is cited from the OpenOCD User's Guide; the Raspberry Pi Debug Probe documentation page returned HTTP 403 and was not opened.
4. **QEMU limits** (no CMSDK GPIO on mps2-an385; RCC/GPIO/timers unimplemented on stm32vldiscovery) — rulings B2 and C3: emulation plus the course's models is accepted; each limit is stated in the chapter (QEMU's own documentation now cited: MPS2 "Differences between QEMU and real hardware", STM32 "Missing devices") and each hardware step says what is needed to test it.
5. **Analogy proposals** — ruling A3: approved; chapter metadata now says "extension approved under owner ruling A3".
6. **Fictional teaching parts U-MCU-1 and U-BOARD-1** — ruling A2: approved; F1-75, F1-76, F1-78 and F1-79 now say that they are the course's own and name the real items they stand for (RP2350 / STM32F446RE; Pico 2 / NUCLEO-F446RE). Their numbers are exercise values (ruling A4); the 2 % baud-error threshold in F1-75 is now labelled "an exercise value".
7. **Unverified register layouts** — ruling C1: CMSDK UART and timer checked against ARM DDI 0479B; MPS2 addresses and clock against AN385 (DAI 0385D); STM32F1 USART, RCC and GPIO offsets and bits against ST's own CMSIS/LL headers (the reference manual itself could not be opened); SysTick against the Armv7-M ARM (DDI 0403E.e); JTAG TAP and SWD format against ADIv5 (IHI 0031G). Only the IEEE 1149.1 text (sold; catalogue page read) and the CMSDK timer's RELOAD + 1 period and clock (QEMU model only) remain unverified.
8. Other rulings checked: A1 (no trimming for length), A5 (recorded runs kept: host models re-run with identical output, recorded logs restored), A8 and A10 (no committed keys and no `LicenseRef-Uni-Lab` placeholder in the HW303 lab folders; nothing to change), C2 (new sources added only when opened), C4 (status follows the QA records).

## Verification pass

Third verification pass, 2026-10-25, with web access. Dossiers `university/_dossiers/F1-73..F1-79.dossier.html` and QA records `university/qa/F1-73..F1-79.json` written; the earlier interrupted WIP edits of F1-73 (commit 90c4f8f) were re-checked against freshly opened documents and completed.

**Opened (all on 2026-10-25):** Arm DUI 0552A (Cortex-M3 Generic User Guide), DDI 0403E.e (Armv7-M ARM), DDI 0479B/C (CMSDK TRM), DAI 0385D (AN385), DDI 0337H (Cortex-M3 TRM), IHI 0031G (ADIv5) — all through Arm's documentation-service API and PDFs; Raspberry Pi RP2350 Datasheet (build 80d627281ed5, 01/10/2026); QEMU documentation (11.1.50 pages) and QEMU v8.2.2 source files; C++ working draft (eel.is, 2026-08-23) and N4861; RISC-V Privileged Architecture (library 20260120); GNU ld manual (2.47) and lld notes; Clang attribute and command-line references; GCC Common Attributes and Developer Options; GDB manual (remote protocol), LLDB remote/command-map pages, OpenOCD User's Guide; CMSIS-Core pages and Open-CMSIS-SVD; ST's `stm32f100xb.h` and `stm32f1xx_ll_gpio.h`; Zephyr "Threads"; TI SZZA036C and SCEA046B; OpenStax University Physics Vol. 3 §9.7; KiCad 9.0 Schematic Editor manual; IEEE 1149.1-2013 catalogue page.

**Not opened:** STMicroelectronics RM0041/datasheets (server refused, curl and WebFetch), Elecia White "Making Embedded Systems" (O'Reilly, HTTP 403 twice), Raspberry Pi Debug Probe documentation (HTTP 403 twice), Analog Devices MT-101 (no response twice). Book catalogue pages only (text not read): Yiu (3rd ed., 2013), Harris and Harris ARM edition (2015), Horowitz and Hill (TOC page), CS:APP (front page). Their former claim tags were re-sourced to opened documents and the book entries were removed from the chapters' Sources lists.

**Corrected (main items):** Cortex-M3 vs Armv7-M reset address of the vector table (VTOR reset rule); MCU program memory can be external flash (RP2350); SRAM-at-power-up claim replaced by the Armv7-M reset pseudocode; QEMU RISC-V `virt` reset path confirmed from QEMU source; C++20 forward-progress rule cited to N4861; "peripherals start with clocks off" made chip-specific; invented datasheet examples in F1-79 (VOH, typical pull-up) replaced with RP2350 values; Cortex-M3 breakpoint count (6 instruction comparators, Code region); watchpoints asynchronous; several "unverified" register boxes turned into sourced text. Per-chapter lists are in the QA records.

**Left unverified (boxes):** F1-74 STM32F100 boot mode/SRAM size/peripheral list and flash wait states; F1-75 UART receiver tolerance, STM32 clock-enable reset values and wait states; F1-76 CMSDK timer period and clock on real MPS2, eye/inductance averaging; F1-77 IEEE 1149.1 text; F1-78 ST document split and timer buffering; F1-79 board-ID resistors, decoupling inductance, LED colour vs forward voltage, STM32 pin ratings. All "untested on hardware" boxes stay (ruling C3) and name the kit hardware needed.

**Labs:** F1-78 and F1-79 re-run with `run_lab.sh` (container g++ is now 15.2.0): outputs identical, logs restored (A5). Host models of F1-75/76/77 re-run in a scratch copy: identical. QEMU/clang/LLDB/RISC-V steps of F1-73..F1-77: lab not re-run: toolchain missing (clang, ld.lld, llvm tools, qemu-system-arm/riscv64, lldb, riscv64-linux-gnu-g++ not installed at the end of the pass); recorded files kept.

**Still open:** listing header comments in F1-73 `main.cc`, F1-74 `main.cc`, F1-75 `probe.cc`, F1-76 `blink.cc`, F1-77 `tap.cpp`, `app.cc`, `app_v2.cc` still say "unverified"/"pending verification" (a note under each listing explains); update them at the next re-run of those labs, once the QEMU/clang toolchain is installed. ST's documents should be opened when the vendor site is reachable.

