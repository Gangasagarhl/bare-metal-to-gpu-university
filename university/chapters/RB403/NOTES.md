# RB403 — Building a robot OS image, and safety: author's notes

Chapters F9-65 to F9-71. Labs are in `university/labs/F9-65` to `university/labs/F9-71`.
Every lab passes `university/labs/run_lab.sh university/labs/<ID>` from the repo root.

## Build environment (as printed in the logs)

- Cloud build container: a 4-vCPU virtual machine.
  - Kernel 6.18.44-fc-v114, `PREEMPT_DYNAMIC`, no PREEMPT_RT.
  - Runs as root, so `SCHED_FIFO` and CPU affinity are allowed.
  - Real-time throttling: `sched_rt_runtime_us` = 950000 of 1000000.
  - No isolated CPUs.
- Tools:
  - g++ 13.3.0
  - aarch64-linux-gnu-g++ 13.3.0
  - GNU ld 2.42
  - QEMU 8.2.2
  - Python 3.13.16
  - sgdisk 1.0.10
  - dosfstools 4.2
  - mtools 4.0.43
- Not available:
  - `/dev/kvm`
  - a Linux kernel source tree or image
  - `dtc`
  - Buildroot or Yocto
  - ROS 2
  - Gazebo
  - USB devices
  - any robot hardware

  As a result, no Linux robot image was built or booted.

## Course-wide decisions taken (owner to confirm)

1. **The "robot OS image" is the course's bare-metal AArch64 kernel, run on QEMU virt as the course board.** Booting a Linux image was impossible here. Each chapter says so openly. The Linux image pipeline appears only as text, in unverified boxes.
2. **The kernel base was copied from DR403's F4-31 lab.**
   - Copied files: `start.S`, `kbase.h`, `kbase.cc`, `kernel.ld`, `fdt.h`, `minidtc.py`.
   - Each copy has one provenance line added. The copies live in `labs/F9-67/`.
   - The labs of F9-68 to F9-71 source the shared helpers in `labs/F9-67/rblib.sh`. The run.sh of F9-69 rebuilds the F9-67 image from its sources.
   - If DR403's files change, these copies do not follow.
3. **The devicetree uses the vendor prefix "course,"** (`course,robot-board`, `course,usb-camera`). It is the course's own invented prefix and not a registered vendor.
4. **QEMU stands in for the e-stop.** On virt with `acpi=off`, the monitor command `system_powerdown` pulses the gpio-keys power key on PL061 line 3. This behaviour was observed in the runs; the QEMU documentation was not opened. The chapters state that this is not a real e-stop.
5. **RB304 terms** (Jitter, PREEMPT_RT, Wake-up latency, Real-time throttling, Threaded interrupt handler, Latency histogram):
   - F9-65 to F9-67 link to RB304 *chapters*, because RB304's glossary.json did not exist when they were written.
   - It exists now, so F9-70 links `#gl-real-time-throttling` directly.
   - "Latency histogram" is defined in both RB304 and RB403. build.py merges duplicates, and RB304's definition wins; F9-66 is added to its chapter list.
6. **Proposed new analogies for the registry** (faculty F9: bicycle and body):
   - reflex arc = the microcontroller loop (F9-65, F9-66);
   - the image = the bicycle prepared and packed, with its ticked checklist (F9-67);
   - getting on the bicycle in order, brakes checked before pushing off = start-up order and readiness (F9-68);
   - the riding partner who shouts "stop" when you stop answering = watchdog (F9-69);
   - the cycle computer's recording and its gap = flight recorder (F9-70);
   - the living-room trainer versus the gravel road, and the wrongly set wheel size = sim-to-real and a calibration error (F9-71).
7. **Image builder for Linux robot images: Buildroot, Yocto, or distribution packages.** This is left to the owner and the MP6 teams (F9-67 unverified box). Neither tool was run, and the chapters give no command or variable of either.
8. **Safety position.** Every chapter states that the hard-wired e-stop chain has no software in its path. F9-69 states that the course's supervisor is a teaching model, not an assessed safety function. Standards are cited by title only:
   - ISO 13850
   - IEC 60204-1
   - ISO 13849-1
   - IEC 61508
   - IEC 61800-5-2
   - ISO 10218
   - ISO 13482

   No requirement of any of them is claimed (AH-6). Faculty or safety-owner review is recommended before teaching F9-69.

## Per chapter

### F9-65 Anatomy of a robot software stack

- **Listings run:**
  - `stack.cpp` on `stack.in`: pass, exit 0. CPU 3 is above the RM bound, but RTA shows that every deadline is met.
  - Forensic `stack_forensic.in`: exit 1, expected (cpu1 overloaded, the detector misses its deadline).
  - `forensic_diff`: the diff of the two inputs.

  All are deterministic.
- **Unverified:**
  - The claims about end-to-end chain analysis (reaction time and data age), beyond the Σ(T+R) estimate.
  - Budgets in `stack.in` are invented for teaching, and no real processor is named.
  - The Liu & Layland bound and RTA are cited by title only.
- **Safety box:** a stack diagram is not a safety design.
- **Owner:** confirm the layer list used as the course's reference stack (eight layers).

### F9-66 Choosing the OS: Linux with PREEMPT_RT, an RTOS, or both

- **Listings run:**
  - `latency.cc`, measured on the VM; sanitizer run plus `-O2` timing runs:
    - SCHED_OTHER and SCHED_FIFO;
    - idle and busy;
    - hog at FIFO 90 (forensic).

    These are measurements of a VM without PREEMPT_RT. The prose avoids quoting exact numbers that vary between runs.
  - `decide.cpp` (decision matrix with sensitivity check): pass, deterministic. Result: linux_plus_mcu 47, linux_rt 41, rtos excluded; 0 of 10 weight changes flip the choice.
- **Unverified boxes:**
  - Linux option and parameter names (CONFIG_PREEMPT_RT, isolcpus, nohz_full, irq affinity).
  - remoteproc, rpmsg and OpenAMP.
  - Timer hardware and SMM statements.
  - Untested on hardware: no PREEMPT_RT kernel and no robot computer.
- **Owner:** the chapter recommends "Linux with PREEMPT_RT plus a microcontroller" for the course robot. The weights in `decide.in` are the author's; confirm them, or replace them with the faculty's.

### F9-67 Building the image: kernel configuration, drivers, devicetree

- **Listings run (all pass; QEMU 8.2.2; untested on hardware):**
  - `mkconfig.py` (resolve, and the dropped option).
  - kernel build.
  - minidtc.
  - reproducibility (two builds, identical Image SHA-256).
  - `mkimage.sh` (GPT with boot/rootfs-b/logs, FAT boot partition, manifest).
  - boot with `sha256sum -c`.
  - five timed boots.
  - Forensic boot: exit 6, expected (refuses to arm).
  - `forensic_diff`.
- **Seen in this build and documented:** `sgdisk -q` (1.0.10) wrote no partition table. The lab drops `-q`.
- **Unverified boxes:**
  - Linux Kconfig resolution and `merge_config.sh`.
  - The GPIO binding convention.
  - Linux sysfs and debug paths.
  - The image-builder decision.
  - Two untested-on-hardware boxes: no SD card, no real boot loader, no Linux pipeline.
- **Owner:** whether a later revision should build a real Linux image (requires a kernel tree and toolchain in the build container).

### F9-68 Middleware deployment and start-up order

- **Listings run:**
  - `startup.cpp` on `startup.in`: pass.
  - `startup_camfail`: pass.
  - `startup_cycle`: exit 1, expected.
  - `forensic_startup` (release 5): pass.
  - `bridgefail_r4`: exit 1, expected.
  - `bridgefail_r5`: pass, which is the forensic answer.
  - `launcher.cpp`: real fork, exec, pipe and poll, on the container; wall-clock times vary between runs.
- **Unverified boxes:**
  - systemd directives (Requires/Wants/After, Type=notify, sd_notify, Restart, systemd-analyze critical-chain).
  - ROS 2 lifecycle states, launch event handlers and DDS discovery behaviour.
  - Two untested-on-hardware boxes.
- **Data:** unit times in `startup.in` are design estimates, not ROS 2 measurements.
- **Owner:** whether `camera_driver` belongs on the course robot's safety path. The chapter treats the camera as optional, using degraded mode via `after=`.

### F9-69 Watchdogs, emergency stops and safe states

- **Listings run:**
  - `safety.cpp` on `safety.in`: pass, 0 invariant violations, deterministic.
  - Forensic `safety_forensic.in`: pass. The watchdog never fires, with a stale command for 400 ms.
  - `watchdog.cpp` (real threads): pass. The detection gap was within T+P in the recorded run; it varies between runs.
  - E-stop test (`estop_test`): the F9-67 image is rebuilt, and `press.py` types `system_powerdown` after 1 s. Result: "E-STOP at cycle …", 0 non-zero actuator commands after the press, exit 0. QEMU only; untested on hardware.
- **Found during the build (not in a recorded output):**
  - A press at 0.0 s can arrive while the image is starting. The image then reads the line as PRESSED at stage 3, arms anyway, and goes safe in cycle 0.
  - The chapter labels this a weakness and makes fixing it a lab step (refuse to arm while pressed). `robot.cc` itself was not changed, so F9-67's outputs stay as they are.
  - Owner: consider making refuse-while-pressed the default in a later revision.
- **Unverified boxes:**
  - The Linux watchdog device API.
  - Standards and stop categories (all from memory; the standards are paywalled).
  - Two untested-on-hardware boxes.
- **Safety boxes:**
  - At the top of the chapter: the software is a teaching model and not a safety function.
  - In the hardware section: no software e-stop, test e-stop each session, qualified people for electrical work.

### F9-70 Logging, telemetry and forensics (course forensic case)

- **Listings run:**
  - `flightrec_test`: PASS.
  - `freeze.cc`:
    - sanitizer run (correctness only);
    - `freeze_shared` (`./freeze 1 1`): one gap of 2000 ms whose edges match the simulated camera log;
    - `freeze_isolated` (`./freeze 2 1`): no gap over 5 ms.
  - `machine.out` (throttling values, empty isolated-CPU list).

  These are real SCHED_FIFO measurements on the VM.
- **Honest limits stated in the chapter:**
  - The USB camera driver is simulated by a thread, and its "kernel log" lines are written by the program.
  - No kernel trace was recorded.
  - "Isolated" means only "not shared with the camera thread"; isolcpus was not configured.
- **Unverified boxes:**
  - Real Linux evidence: dmesg, ftrace/trace-cmd/perf sched, threaded IRQs on PREEMPT_RT, changes to throttling.
  - Linux CPU isolation parameters.
  - Two untested-on-hardware boxes.
- **Owner:** the course card says "kernel log plus trace". A later build with a real kernel and tracing should replace the simulated evidence with real ones.

### F9-71 Sim-to-real

- **Listings run (all pass, deterministic; all "robots" are models):**
  - `sim2real.cpp` on `sim2real.in` (8 cases).
  - `margin` (`sim2real_margin.in`): confirms the computed delay margin π/160 s ≈ 19.6 ms. The cliff falls between 18 and 19 ms of added delay. It also shows the reversed-CPR error.
  - `forensic_sim2real` (CPR 2048 true vs 1024 assumed: true 30, measured 60).
- **Unverified boxes:**
  - The delay-margin formula and loop-shaping (textbooks by title only).
  - Simulator and ROS 2 clock statements (no simulator available).
  - Two untested-on-hardware boxes.
- **Owner:** the BR-08 lab "same controller in simulation and on the supervised kit" is an extension, gated by MP6 R3. It was not done here.

## Glossary

`glossary.json` has 48 entries, generated from the seven jargon boxes:

- `source` names the first claim tag of each definition.
- `chapters` lists every RB403 chapter that links the term.

Every `#gl-` link in the seven chapters resolves to a term in some course's glossary.json, either RB403's or an earlier course's: RB101, HW204, OS302, OS305, RB303, RB304, and others.

## Claims not verified (summary)

- No external document was opened (dossier gate G1 open). Every D-source is cited by title only.
- Every number in the chapters is one of:
  - printed by our own runs, and labelled as a measurement where it is one;
  - derived from those numbers in a worked example;
  - an invented design value, labelled as such.
- No hardware number of any real part is given.
