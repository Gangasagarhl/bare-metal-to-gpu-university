# DN302 PX4 on NuttX — build notes (Author + Lab Engineer)

Build date 2026-10-10. Container: Linux x86_64, g++ 13.3.0. No internet, no PX4 source, no NuttX source, no simulator, no flight controller.
No source document was opened. Every D-source is cited by title only, with "dossier gate G1 open".

## Files

- Chapters: `F10-20.html` … `F10-26.html` (7 fragments). Each has 22 sections in template order, 2 inline SVG figures, 9 check-yourself questions, a forensic lab with an answer key, and 2 unverified boxes.
- `glossary.json`: 46 four-part entries, generated from the jargon boxes. Terms that already exist elsewhere are linked, not redefined: POSIX, Real-time factor, Innovation, Probe, Bring-up, SPI, I2C, PWM and others.
- Labs: `university/labs/F10-20` … `F10-26`. All seven pass `run_lab.sh` (rc=0), re-run at the end of the build.
- Fragment validation: tags balanced (html.parser), ids prefixed, no URLs, no `<script>`, sections in order, sources cited and defined, all `data-src`/`data-run` files present, all internal and glossary links resolve against a scratch build. 0 problems in all seven.
- `python3 university/build/build.py`: no PROBLEM line refers to DN302 links or ids. The remaining PROBLEM lines belong to other courses.

## Listings run (statuses)

| Chapter | Program / step | Status |
|---|---|---|
| F10-20 | posix_rate (SCHED_FIFO periodic thread + POSIX mq) | pass; lateness numbers vary per run, and the text quotes none |
| F10-20 | flat_build (flat vs protected memory model) | pass |
| F10-20 | forensic_flat | pass (the evidence is the misbehaviour) |
| F10-21 | wq_sim (work-queue simulator, configs A and C) | pass |
| F10-21 | forensic_wq (config B) | pass (evidence) |
| F10-22 | uorb_demo | pass |
| F10-22 | uorb_threads | pass |
| F10-22 | run.sh: uorb_threads_tsan | pass (ThreadSanitizer clean) |
| F10-22 | forensic_yaw | pass (evidence) |
| F10-23 | sitl_lockstep (two processes, socketpair, lockstep) | pass |
| F10-23 | run.sh: repeat (3 runs byte-identical) | pass |
| F10-23 | forensic_sitl (ms/µs bug) | pass (evidence) |
| F10-24 | shell (+ shell.in) | pass |
| F10-24 | tilt_guard_test | pass, 8/8 |
| F10-24 | forensic_shell | pass (evidence) |
| F10-24 | forensic_test | **expected-fail**: exit 1, one test fails by design |
| F10-25 | run.sh: flight_sim, header_bytes, ulog_info, params_diff, innov_bad, innov_good | pass |
| F10-26 | board_check (draft description) | **expected-fail**: exit 1, the draft has 2 errors by design |
| F10-26 | board_fixed, bringup | pass |
| F10-26 | forensic_bringup | pass (evidence) |

**Untested on hardware / untested in this build:**
- every PX4 and NuttX command, build target, file and API named in the chapters;
- NuttX simulator runs (F10-20);
- PX4 SITL (F10-23 part B, F10-24 part B, F10-25 part B);
- the bench flight controller with propellers off (F10-24 part C, the course lab);
- the real board port (F10-26 part B).

Each is marked in an unverified box and, for hardware, also in a "Watch out — untested on hardware" box (F10-24, F10-26).

## Unverified boxes (2 per chapter, 14 in total)

- **F10-20.**
  - NuttX facts: task_create/posix_spawn and what a pthread shares with its task, the build-mode names and CONFIG_BUILD_FLAT/PROTECTED/KERNEL, the priority range and default policy, and which mode PX4 boards use, among others.
  - Lab step 6: building and running on NuttX.
- **F10-21.**
  - ModuleBase, px4::ScheduledWorkItem/WorkItem, ScheduleOnInterval/ScheduleNow, SubscriptionCallbackWorkItem/registerCallback, the work-queue list and priorities (wq:rate_ctrl, wq:nav_and_controllers and others), and the status commands.
  - Lab step 6 in SITL.
- **F10-22.**
  - .msg files, ORB_ID, ORB_QUEUE_LENGTH, uORB::Publication/PublicationMulti/Subscription/SubscriptionInterval/SubscriptionCallbackWorkItem, and the C API and shell tools.
  - Lab step 6 in SITL.
- **F10-23.**
  - `make <vendor>_<board>_<label>`, `make px4_sitl gz_x500 / gazebo-classic / jmavsim`, `Tools/setup/ubuntu.sh`, the lockstep default, the HIL_SENSOR/HIL_ACTUATOR_CONTROLS interface, the `pxh>` prompt and `commander takeoff`, `hrt_absolute_time()`.
  - Lab part B.
- **F10-24.**
  - ModuleBase/ScheduledWorkItem/ModuleParams, DEFINE_PARAMETERS, SubscriptionCallbackWorkItem, `px4_add_module`, `default.px4board` Kconfig, ROMFS init.d scripts, PARAM_DEFINE_FLOAT/YAML, the 16-character parameter-name limit, the `vehicle_attitude` (quaternion) and `parameter_update` topics.
  - Lab parts B and C (the course lab).
- **F10-25.**
  - ULog byte layout and message types, pyulog tools, Flight Review, PlotJuggler, SDLOG_* parameters, estimator topic names, the definition of PX4's test ratio.
  - **EKF2_GPS_POS_X/Y/Z**, EKF2_IMU_POS_*, EKF2_GPS_P_GATE, EKF2_GPS_DELAY, EKF2_GPS_P_NOISE.
  - Lab part B.
- **F10-26.**
  - The PX4 board folder layout (default.px4board, board_config.h, spi.cpp with initSPIBus/initSPIDevice, timer_config.cpp, nuttx-config/…, rc.board_sensors, firmware.prototype), the bootloader, the probe reporting.
  - Lab part B.

## Decisions for the owner

1. **Course-forensic parameter names.** The brief says "Parameter names may come only from the PX4 documentation". The documentation could not be opened, so the forensic log uses the course's own names (`UNI_GPS_OFF_X` and the other `UNI_…` parameters), clearly labelled as not PX4's. The PX4 name (believed `EKF2_GPS_POS_X`) appears only inside an unverified box. Proposal: when the Source Researcher confirms the PX4 names, either keep `UNI_…` with a mapping table, or rename them in `flight_sim.cc` and regenerate. The forensic logic does not depend on the name.
2. **ULog-style format.** `ulog_lite.h` follows the ULog structure from memory: header magic `ULog 01 12 35`, size+type messages, and only the F/I/P/A/D/L message types. Byte compatibility with pyulog or Flight Review is **not verified**, and the chapter says so. A verifier should check it with a real `.ulg` file. Option: once verified, make the writer compatible so learners can open course logs in PX4's tools.
3. **No real PX4/NuttX in the build.**
   - All labs use the course's models: the POSIX threads stand-in for NuttX tasks, the work-queue simulator, uorb_lite, mini_px4, the lockstep SITL, the ULog-style logs, and the board checker and bring-up model.
   - The real-tool parts are unverified boxes.
   - The course lab (PX4 SITL, then the bench flight controller) and practical exam P need a machine with PX4 and a flight controller before G5 can be "tested" rather than "untested-on-hardware".
   - This needs owner approval under AH-26.
4. **Our own module parameter `TG_MAX_TILT`** (F10-24) is named in a PX4-like style but is ours. The chapter says this. Topics in the course runtime end in `_model` so they are not mistaken for PX4 topics.
5. **Analogy mappings proposed** (F10 world "carrying a tray of drinks while walking"; uORB = notice board is already registered). Proposals:
   - RTOS scheduler = the body's rule that the balance reflex always wins over chatting; flat build = everyone carries from one shared tray (F10-20);
   - work queue = one helper's list of quick jobs, done in order; work item = one quick job on that list (F10-21);
   - queue length = how many of the newest notes stay pinned; generation counter = the running number written on each note (F10-22);
   - SITL = rehearsing the walk in the hallway with plastic cups; lockstep = the coach who says "step" and waits (F10-23);
   - a module = one helper with one job who reads notices and pins up their own (F10-24);
   - a ULog = a photograph of every notice each time one changes, plus the settings at the start (F10-25);
   - porting = learning a new tray for the same walk (F10-26).
6. **Chapter length.** Prose is about 5,200–6,800 words per chapter (L4 target about 5,000). F10-20 is the longest (about 6,800); trim it if the editor wants a tighter chapter.
7. **Exams.**
   - Q: the check-yourself questions are a starting pool.
   - F: each chapter's forensic lab, plus the course forensic in F10-25.
   - P: "add and test a module in SITL" is previewed in F10-24's lab. No separate exam file was written, because the brief did not list exam files as deliverables.
8. **Link to F10-27** (DN303) for "next" in F10-26. It resolves through the catalogue anchor until DN303 is written.

## Notes for verifiers

- Numbers in the prose come only from this build's runs (R-sources) or from arithmetic shown in the text. Two scratch runs are quoted as such: F10-25 with the 0.6 m offset (fused 170 of 170, max ratio 0.89, fit 0.01 m, max error 0.84 m), and F10-26's checker experiments (Check yourself 5 and 7).
- F10-20's `posix_rate` lateness is machine-dependent, and no value is quoted in the prose.
- No hardware numbers (CPU load, timing on a flight controller, SD write rates) are given anywhere.
- No commercial kits are named.
