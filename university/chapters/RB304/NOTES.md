# RB304 — Real-time systems for robots: author notes

Chapters F9-45 to F9-51. Level L3–L4, 4 credits. Prerequisites: RB303, OS305, SP203.
Labs are in `university/labs/F9-45` to `university/labs/F9-51`.

Each lab passes `university/labs/run_lab.sh university/labs/<ID>` with exit code 0 (runs of 2026-10-10; F9-47 and F9-51 were re-run at the end of the build and gave byte-identical outputs).

Every fragment passes the local checks:
- html.parser balance;
- ids prefixed with the chapter id;
- no URLs;
- no `<script>`;
- section order (21 template sections plus Answers, forensic key as an h3 inside Answers);
- every source reference resolves and every source is cited;
- every `data-src` and `data-run` file exists;
- every glossary link resolves (39 RB304 terms in `glossary.json`; other links go to OS303, OS305, HW204, OS301, RB303 and RB302 terms, among others).

`python3 university/build/build.py` reports no PROBLEM line for F9-45 to F9-51. The remaining PROBLEM lines name other courses' glossary links.

## Build environment (as recorded in the logs)

- **The build container.**
  - Linux 6.18.44 in a Firecracker virtual machine, 4 CPUs, run as root.
  - The kernel is built `PREEMPT_DYNAMIC` with `CONFIG_PREEMPT_NONE=y`, `HZ=250` and high-resolution timers. There is **no PREEMPT_RT** (F9-47 `config.out`).
  - Clock source: `tsc`.
  - Threaded handlers `irq/24` and `irq/25` run at SCHED_FIFO 50.
  - `RLIMIT_RTPRIO` 0/0 and `RLIMIT_MEMLOCK` 8 MiB.
  - Real-time throttling: 950,000 / 1,000,000 µs.
- **g++ 13.3.0.** Host programs are built by `run_lab.sh` with `-fsanitize=address,undefined`.
  - Timing programs are `.cc` files, built by each lab's `run.sh` twice: a sanitizer smoke run, and an `-O2` build without sanitizers for the measurement.
- **For F9-51:** clang/LLD 18.1.3, and QEMU 8.2.2 `mps2-an385` with `-icount shift=0,sleep=off`. These are the tools of OS305 F3-39.
- **Not installed, so nothing about them was compiled or run:** ROS 2, ros2_control, micro-ROS, cyclictest/rt-tests, kernel tracing tools, a PREEMPT_RT kernel, any real board, IMU or motor.

## Measured versus deterministic outputs

**Measured on the build container (AH-23).** These change from run to run, and the prose quotes the outputs that are in the repository:
- F9-45 `loop_abs`, `loop_rel`, `loop_smoke`;
- F9-46 `deadline`, `deadline_smoke`;
- F9-48 all six outputs;
- F9-49 all three;
- F9-50 `cm_rt`, `cm_rt_pinned`, `cm_rt_smoke`.

**Warning to maintainers:** re-running these labs changes the numbers. The chapter prose, figures (F9-48 Figure 2, F9-49 Figure 1) and answer keys must then be updated to match the new `.out` files.

**Deterministic.** Identical on every run:
- F9-45 `timing_terms`;
- F9-46 `sched_sim`, `analysis`, `rt_limits`;
- F9-47 all four;
- F9-50 `cm_demo`, `cm_forensic`, `hw_diff`;
- F9-51 all six (virtual time).

## Listings run

**Nothing was run on real hardware.**

| Chapter | Run | Status |
|---|---|---|
| F9-45 | timing_terms.cpp (constructed trace) | pass, deterministic |
| F9-45 | loop_1khz.cc absolute vs relative 1 kHz loop | pass. Measured: drift 0.058 ms against 949 ms over 4 s |
| F9-46 | sched_sim.cpp RM vs EDF; analysis.cpp (LL, hyperbolic, RTA with B and J, EDF dbf) | pass, deterministic |
| F9-46 | deadline_linux.cc SCHED_DEADLINE admission and period | pass, measured. Run as root |
| F9-47 | rt_probe.cpp, config step (/proc/config.gz), preempt_model.cpp (model, forensic rt-prio40) | pass |
| F9-47 | Two-kernel histogram comparison (course lab) | **untested on hardware**: the container cannot boot a second kernel. The chapter gives a model and a procedure |
| F9-48 | latency.cc: idle/load × OTHER/FIFO, fresh/reuse work | pass, measured |
| F9-49 | uexec.h + rt_node.cc bad/fixed/smoke | pass, measured. `run.sh` asserts 0 allocations in the fixed run |
| F9-49 | The same structure in a real ROS 2 node | **untested** (no ROS 2) |
| F9-50 | uctl.h, sim_hw.h, controllers.h; cm_demo, cm_forensic, hw_diff | pass, deterministic |
| F9-50 | cm_rt.cc 1 kHz SCHED_FIFO 80, unpinned and pinned | pass as a run. **Requirement R1 and R2 FAIL** (expected-fail on this VM); R3 PASS |
| F9-50 | Meeting R1–R3 on PREEMPT_RT hardware; a real ros2_control interface | **untested on hardware** |
| F9-51 | imu_node.cc on uRTOS in QEMU; agent.cc; repeat; forensic variant; size | pass in emulation. **Untested on hardware** |
| F9-51 | micro-ROS client and agent publishing a ROS 2 topic (course lab) | **untested** (no micro-ROS) |

## Unverified boxes (claims from memory, to be checked when gate G1 is closed)

- **F9-45.** Timer slack: default 50 µs for normal threads, none for real-time threads; `prctl` timer-slack operations.
- **F9-46.** SCHED_DEADLINE details:
  - it uses a constant bandwidth server;
  - the exact admission rule;
  - the attribute struct fields and the header;
  - the error code on refusal.

  What was run is recorded; the meaning is unverified.
- **F9-47.** Three boxes:
  - PREEMPT_RT mechanisms, `CONFIG_PREEMPT_RT`, mainline status, `/sys/kernel/realtime`, `threadirqs`;
  - boot options `isolcpus`, `nohz_full`, `rcu_nocbs`, `irqaffinity`, `preempt=`, and the default irq thread priority 50;
  - `pthread_mutexattr_setprotocol` / `PTHREAD_PRIO_INHERIT` (not compiled).
- **F9-48.** Four boxes:
  - cyclictest and rt-tests;
  - timer slack, `PR_SET_TIMERSLACK`, `/proc/<pid>/timerslack_ns`;
  - glibc's adaptive mmap threshold and `M_MMAP_THRESHOLD`;
  - ftrace and its latency tracers. `CONFIG_PREEMPT_TRACER` is not set here.
- **F9-49.** Two boxes:
  - all rclcpp executor and callback-group API names and behaviours;
  - realtime_tools, ros2_tracing, message allocators, DDS allocation.
- **F9-50.** All ros2_control names:
  - SystemInterface and its lifecycle methods;
  - the export functions;
  - ControllerInterface;
  - the `<ros2_control>` tag;
  - `update_rate`;
  - diff_drive_controller, joint_state_broadcaster and spawner;
  - the RT priority handling of the controller manager.
- **F9-51.** Two boxes:
  - micro-ROS architecture: rcl/rclc, rclc_executor, DDS-XRCE / Micro XRCE-DDS, agent transports, supported RTOSes, build-time entity limits, stream reliability;
  - the CRC-16 check value 0x29B1 as the published value of CRC-16/CCITT-FALSE.

  Inline: micro-ROS time synchronisation, and the cause of the entity-creation failure.
- **All D-tier sources are "title only — not opened during this build (dossier gate G1 open)":** Linux kernel docs, man-pages, POSIX, Buttazzo, Liu & Layland, ROS 2 docs, ros2_control docs, micro-ROS docs, OMG DDS-XRCE, C++20.

## Analogy proposals (F9: drummer = real-time, bicycle = control loop)

Registered mappings used: the drummer who must hit every beat (F9-45, F9-46, F9-48, F9-49) and the bicycle (F9-45, F9-50, F9-51). New mappings proposed for the analogy register:

1. **F9-46 "who plays first".** The scheduling policy is the band's rule for who plays first when two parts are due together. RM means "the faster part first"; EDF means "whoever's bar ends soonest".
2. **F9-47 "the rider who can drop a daydream".** A preemptible kernel is a rider who can drop any thought when the bicycle wobbles. A non-preemptible section is a thought he must finish.
3. **F9-49 "band with one player for all parts versus one player per part".** This covers executors (players) and callback groups (the parts written for one player). The page-turning drummer is the forensic case.
4. **F9-50 "nerves between brain and muscles".** These are the hardware interfaces. Controllers are the brain's skill, and state and command interfaces are nerve fibres.
5. **F9-51 "a reflex near the muscle".** This is the microcontroller node. The micro-ROS agent is the interpreter at the border.

## Decisions for the owner

1. **Root in the container.**
   - SCHED_FIFO, SCHED_DEADLINE (F9-46) and `mlockall` measurements ran as root. The limit is `RLIMIT_RTPRIO` 0, so an ordinary user would be refused.
   - The chapters say so, and the labs' troubleshooting explains the user limits.
   - Decide whether learners' lab machines will grant real-time priority to a lab group.
2. **VM measurements as teaching evidence.**
   - All timing numbers come from a shared Firecracker VM without PREEMPT_RT. They show the effects (load, priority, page faults, structure) clearly but are not representative of robot hardware.
   - The F9-47 two-kernel comparison and the F9-50 jitter requirement need a real machine. Consider a reference lab PC with a PREEMPT_RT kernel to produce reference histograms.
3. **The F9-50 jitter requirement fails on the build container.** R1 needs ≥ 99.9 % of periods within ±100 µs; measured 96.74 % and 97.86 %. R2 needs no period above 2 ms; measured 9 and 5.
   - This is presented honestly as a platform failure (R3, the code's own time, passes) and as the exam task on real hardware.
   - The R1–R3 values are this chapter's lab plan. The guide says "rate set in the lab plan"; the owner may want to set the official values.
4. **Own frame format "uframe" in F9-51 instead of micro-ROS.** micro-ROS and ROS 2 are not installed. The chapter teaches the node structure (tasks, ring, framing, sequence numbers) with the course's own format and keeps every micro-ROS name in unverified boxes. A real micro-ROS lab needs a supported board and the micro-ROS toolchain.
5. **uRTOS copied unchanged from F3-39 into `labs/F9-51`.** The files are `board.h`, `startup.cc`, `urtos.h`, `urtos.cc` and `os305.ld`. A copy keeps the lab self-contained, but changes to F3-39's uRTOS will not propagate. Decide between copies and a shared lab library.
6. **Miniature frameworks instead of ROS 2 and ros2_control.** These are `uexec.h` (executor and callback groups) and `uctl.h` (hardware interface, controller, controller manager). They use our own names and are marked as such. When ROS 2 is available, a follow-up should port the F9-49 and F9-50 labs to rclcpp and ros2_control and re-verify the unverified boxes.
7. **Glossary overlaps.**
   - RB303 defines "Executor (ROS 2)" and "Callback group". F9-49's jargon box redefines both for this chapter's context, but links to RB303's glossary entries and does not add duplicates.
   - "Response time", "Slack", "Tail latency" and "Page fault" are likewise left to their existing courses.
   - "Percentile" (RB304) sits next to "Median and percentile" (elsewhere).
8. **Course lab "latency histograms with and without PREEMPT_RT on the same machine"** is delivered as a model plus a procedure, with no measured RT histogram. It needs hardware time to complete.
