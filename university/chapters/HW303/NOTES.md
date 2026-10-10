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

## Unverified boxes (per chapter: what to check, and where)

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

All book and specification sources are listed "title only — not opened during this build (dossier gate G1 open)". The Source Researcher must confirm editions and sections.

## Fictional teaching parts (labelled in every chapter that uses them)

- **U-MCU-1.** A teaching microcontroller.
  - Its clock tree is defined by `F1-75/clocktree.cpp`.
  - Its PWM timer rules are defined by `F1-76/pwm.cpp`.
  - Its TIM2 reference-manual excerpt is `F1-78/umcu1_rm_tim2.txt`, implemented by `umcu_tim.cpp`.
  - Its "datasheet" pin limit of 8 mA is set in the F1-79 netlists.
- **U-BOARD-1.** A teaching board, defined by `F1-79/netcheck.in` (rev A) and `rev_b.net` (rev B, the forensic file).
- **The F1-77 JTAG device.** Its IDCODE 0x0EDC0C01 and instruction codes are invented.

## Analogy proposals (F1 restaurant world). Owner approval needed.

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

## Decisions for the owner

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
