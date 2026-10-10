# DN303 — ArduPilot on ChibiOS, and MAVLink: author notes

This course has six chapters, F10-27 to F10-32. It is level L4 and worth 3 credits. The prerequisites are DN301 and OS305.

Each lab is in `university/labs/<ID>`. Each one passes `university/labs/run_lab.sh university/labs/<ID>` with exit code 0. All six were re-run at the end of this build, on 2026-10-10. After that, `python3 university/build/build.py` reported no PROBLEM line that names a DN303 chapter.

The numbers in the prose, figures, worked examples and answer keys were checked against the `.out` files. Some answers predict a result from a scratch change instead of a listing. Two of them were checked with scratch runs in `/tmp/claude-0/work/DN303/`:

- F10-32 Q5: 7 junk bytes and no cut gives 840 records with 7 bytes skipped.
- F10-29 worked example: the most-constrained-first DMA allocation was worked by hand.

The other "predict, then run" answers tell the student to run the change.

## Toolchain and local evidence

- **Host compiler.** g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0, used with `-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined`.
- **Firmware build (F10-27).** Ubuntu clang 18.1.3 and Ubuntu LLD 18.1.3, targeting thumbv7m-none-eabi. The firmware ran on QEMU 8.2.2, `mps2-an385` (Cortex-M3), with `-icount shift=0,sleep=off`.
- **Python.** Python 3.13.16 and numpy 2.5.3, for the generators in F10-29 and F10-30 and the log tools in F10-32.
- **Not available.** There was no ArduPilot source tree, no SITL, no ChibiOS, no MAVLink library (pymavlink or any other), no ground station and no flight controller. So every claim about those projects is either tagged with a source that was not opened, or placed in an unverified box.
- **Invented teaching items.** Each one is labelled as invented in the chapters:
  - F10-27: the teaching HAL on uRTOS/QEMU. uRTOS files are unchanged copies from `labs/F3-39`.
  - F10-29: the pretend MCU U-MCU1, the board U-FC1 (the same name as DN202's pretend board) and its own board-file format.
  - F10-30: the U-link protocol and dialect.
  - F10-31: the mini-SITL over loopback UDP.
  - F10-32: the UDF log format.
  - None of these use real ArduPilot, ChibiOS or MAVLink keywords, IDs, marker bytes or parameter names. The parameter-like names in F10-32 (`U_GPS_MINSATS` and others) are invented.

## Listings run

**Nothing ran on hardware.** The F10-27 firmware ran only in QEMU.

**Expected failures:**
- F10-27 `forensic` exits with 1 by design.
- F10-29 `gen_bad` exits with 1 by design.

| Chapter | Run | Status |
|---|---|---|
| F10-27 | vehicle_host | pass (host backend; V lines at ticks 0/10/20/30) |
| F10-27 | fw (run.sh, clang/QEMU) | pass in QEMU, untested on hardware (fast high-water 72/1024, telem 140/1024, guard 0, PASS) |
| F10-27 | compare | pass (vehicle lines identical on both backends, 5 lines) |
| F10-27 | size | pass (.text 3844, .data 24, .bss 9244) |
| F10-27 | forensic | expected-fail, exit 1 (LOG_LINE_BYTES=1400: high-water 72, 21 guard words written) |
| F10-27 | sweep | pass (64/600/948/1000/1400 bytes) |
| F10-27 | driver_states | pass (1 state error caught) |
| F10-28 | sched_table | pass (design B: telemetry 166/500, logging 649/1000, 0 overruns) |
| F10-28 | modes | pass |
| F10-28 | perf_log | forensic evidence, exit 0 |
| F10-29 | gen | pass (rev A, all DMA assigned) |
| F10-29 | board_use | pass (self-check PASS) |
| F10-29 | gen_bad | expected-fail, exit 1 (5 errors, no header) |
| F10-29 | flash_layout | pass (application at 0xC000, 126 KiB spare) |
| F10-29 | forensic_gen, forensic_use | forensic evidence, exit 0 (UART3_RX without DMA; self-check WARN) |
| F10-30 | gen | pass (CRC check value 0x6F91; crc_extra table) |
| F10-30 | frame_demo | pass (20- and 29-byte frames; 1 CRC error, 3 noise bytes, gap of 2) |
| F10-30 | mission_upload | pass (run A 320 ms, run B 820 ms) |
| F10-30 | forensic_gen, forensic | forensic evidence, exit 0 (crc_extra 64 vs 58; 6 CRC errors on id 12) |
| F10-31 | router | pass |
| F10-31 | vehicle + companion (run.sh, loopback UDP) | pass; times vary by about 0.1 s between runs |
| F10-31 | lostlink_gen | forensic evidence, exit 0 |
| F10-31 | lostlink_check | answer key, exit 0 (never versus 32.10 s) |
| F10-32 | fly_log, hexdump, read_summary, read_bat, analyse | pass |
| F10-32 | read_damaged | pass (7 bytes skipped, truncation reported) |
| F10-32 | forensic_summary, forensic_events, forensic_gps, forensic_analyse | forensic evidence and answer key, exit 0 |

Every chapter's "Part B" labs are marked untested in this build:
- F10-27: Parts B and C.
- F10-28 to F10-32: Part B (real SITL, board, MAVLink library or log tool).

## Unverified boxes (21 in all)

- **F10-27 (3).**
  - ChibiOS and ArduPilot names (RT/HAL/OSAL APIs, AP_HAL classes, waf, SITL).
  - Supported MCU families and whether an MPU guards stacks.
  - Lab Parts B and C, untested.
- **F10-28 (4).**
  - Directory and library names (ArduCopter, AP_Scheduler, AP_Param, GCS_MAVLink and others) and waf.
  - Loop rate default (400 Hz), the scheduler macro and priority, and the performance record.
  - IMU-synchronised loop, threads and features per board.
  - Lab Part B, untested.
- **F10-29 (4).**
  - hwdef paths, keywords, `chibios_hwdef.py`, the DMA resolver, the board-ID registry and `waf configure --board`.
  - `.apj`, bootloader board file, flash storage scheme.
  - Bootloader boot sequence.
  - Lab Part B, untested.
- **F10-30 (3).**
  - MAVLink 1/2 bytes, the header, signing, the CRC variant and its check value 0x6F91, wire order, truncation, and message, file and generator names.
  - ArduPilot's internal dispatch, stream-rate parameters and radio framing.
  - Lab Part B, untested.
- **F10-31 (4).**
  - Ground-station and companion software names, sysid 255 and component IDs, and the SITL launcher.
  - `FS_GCS_ENABLE`, `SYSID_MYGCS` and the failsafe conditions.
  - Companion wiring and power.
  - Lab Part B, untested.
- **F10-32 (3).**
  - DataFlash markers 0xA3 0x95, FMT 128, the format letters, FMTU/UNIT/MULT, record names, tools, and .tlog/.bin.
  - Logger buffers, threads and dropped records.
  - Lab Part B, untested.

**Sources.** All are "title only — not opened during this build; dossier gate G1 open":
- ChibiOS documentation
- ArduPilot documentation
- ArduPilot source repository
- MAVLink Developer Guide
- MCU datasheet and reference manual (not chosen)
- Liu and Buttazzo real-time texts
- SD Association physical layer simplified specification
- a bipartite-matching algorithms text, still to be chosen

The University Authoring Guide was opened and is cited for safety rules only.

## Decisions for the owner

1. **Course card versus build reality.** The course card asks for an ArduPilot SITL lab, a companion program speaking MAVLink to SITL, and a P exam (a MAVLink client for mission upload in SITL). None of these could run here. Each chapter teaches the mechanism with the university's own programs, and makes the real SITL or MAVLink step "Part B, untested in this build". Please confirm this approach, or schedule a build with ArduPilot and pymavlink installed so that Part B can be run and its outputs recorded.
2. **Lost-link forensic.** The answer key's conclusion depends on the vehicle counting any system-255 heartbeat when no single controlling ground station is configured. This is presented as the mechanism to check against ArduPilot's documentation for `SYSID_MYGCS` and the GCS failsafe, not as a claim about ArduPilot. The Source Researcher should verify it before the chapter claims more.
3. **U-FC1 name.** F10-29 reuses the name of DN202's pretend board U-FC1, with a new pretend MCU, U-MCU1. Keep it for continuity, or rename it.
4. **Word counts.** Counts are prose plus tables, measured by the scratch validator: F10-27 about 7,370; F10-28 6,290; F10-29 5,300; F10-30 5,310; F10-31 5,160; F10-32 4,710. F10-27 and F10-28 are above the L4 target of about 5,000 words. Trimming candidates are F10-27 Layer 3 and the line-by-line tables.
5. **F10-27 correction.** An early draft claimed that stack painting under-reports whenever a buffer is left unwritten. The sweep run (R7) showed the mark stays correct while deeper writes land inside the stack. It under-reports only when the overflow jumps past the stack. The chapter now states the measured behaviour.
6. **Builder warnings outside DN303.** `build.py` reports PROBLEM lines for other courses (for example broken `#gl-…` links from SE courses, an F10-38 run, and a duplicate `gl-finite-state-machine-fsm`). These were left untouched, as instructed.

## Proposed analogy mappings (faculty F10: carrying a tray of drinks while walking)

The registered mappings are used as registered:
- MAVLink = walkie-talkie code words (F10-30)
- failsafe = go back to the meeting point (F10-28, F10-31)

New mappings proposed for the registry:

| Concept | Mapping | Chapter |
|---|---|---|
| HAL | the same carrying habits in different halls | F10-27 |
| scheduler / task table | the waiter's list of other jobs (round list), done in order when time remains | F10-28 |
| task budget | "this will take me at most ten seconds" | F10-28 |
| flight modes | different ways of walking: carrying, clearing, standing by the bar | F10-28 |
| parameters | the house rules written on a card | F10-28 |
| hwdef / board description | the floor plan of a new restaurant | F10-29 |
| DMA stream | a trolley that carries glasses while the waiter does other things | F10-29 |
| bootloader | the restaurant's back door through which the new floor plan is delivered | F10-29 |
| dialect | the code card printed for one restaurant | F10-30 |
| CRC extra | the code card's watermark | F10-30 |
| heartbeat | each waiter clicking the walkie-talkie once a second | F10-30 |
| ground station | the head waiter's desk | F10-31 |
| companion computer | an assistant walking alongside | F10-31 |
| DataFlash log | the waiter's diary | F10-32 |
| FMT record | an entry of the diary's legend | F10-32 |
| telemetry log | the head waiter's notes of what was said on the radio | F10-32 |
