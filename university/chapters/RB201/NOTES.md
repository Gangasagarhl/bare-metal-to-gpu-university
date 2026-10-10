# RB201 — Robot hardware: sensors, actuators, power and buses: author notes

Chapters F9-08 to F9-15, L2, 3 credits. Labs are in `university/labs/F9-08` to `university/labs/F9-15`.
Each lab passes `university/labs/run_lab.sh university/labs/<ID>` with exit code 0. All eight were re-run at the end of this build, on 2026-10-10, and every `.log` says `exit code: 0`. The numbers quoted in prose, figures and answer keys were checked against the `.out` files. Every "predict, then run" answer was checked by running an edited copy in scratch space (`/tmp/claude-0/work/RB201/`). Those scratch results are listed below and are not part of the lab folders.

`python3 university/build/build.py` was run after all eight chapters were written. It reports no PROBLEM line that comes from an RB201 chapter: no broken link, duplicate id, missing listing or missing run. The remaining PROBLEM lines are broken `#gl-` links in other courses.

## Toolchain and local evidence

- **g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0.** Every `.cpp` is built by `run_lab.sh` with `-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined`.
- **Linux UAPI headers, linux-libc-dev 6.8.0-146.146.** `linux/can.h` and `linux/can/raw.h` were opened for:
  - `CAN_SFF_MASK` and `CAN_MAX_DLEN`;
  - `struct can_frame` and `CAN_RAW`.

  They are used by F9-14 (`can_model.h`, `can_probe.cpp`). A `static_assert` checks `CAN_MAX_DLEN == 8`.
- **No hardware.** This build had no motors, encoders, IMUs, LiDARs, cameras, batteries, regulators, CAN interfaces or a robot kit.
  - Every device number comes from the university's own C++ simulators and is labelled as a pretend exercise value.
  - The build container's kernel has no CAN support: `socket(PF_CAN, …)` fails with "Address family not supported by protocol".
- **Sources.** Nothing was opened except the authoring guide ("Honesty first", the F9 safety rules), the curriculum and the Linux headers.
  - Every book, paper, standard and datasheet is cited as "title only — not opened during this build (dossier gate G1 open)".

## Listings run

All runs: exit code 0. "Forensic" means the program generates the evidence pack and the fault is explained in the chapter's answer key. "Untested on hardware" means the program is about a real device that was not present.

| Chapter | Program | Role | Result |
|---|---|---|---|
| F9-08 | `wheel_torque.cpp` | Listing 1: torque at the wheel and the gear-ratio window | pass (OK window 15–40) |
| F9-08 | `stepper_steps.cpp` | Listing 2: step pattern and microstepping | pass |
| F9-08 | `stepper_forensic.cpp` | forensic: lost steps on the return with a part | pass (expected fault shown: home_error 3693) |
| F9-09 | `diffdrive_sim.cpp` | simulator that makes the encoder log | pass |
| F9-09 | `odometry.cpp` + `odometry.in` | Listing: odometry update with midpoint heading | pass (heading 360.008°) |
| F9-09 | `run.sh` → `log_matches_sim` | checks `odometry.in` equals the simulator output | pass |
| F9-09 | `drift_logs.cpp` | forensic: "the robot drifts left" (wrong diameter vs slip) | pass (expected faults shown) |
| F9-10 | `imu_mount.cpp` | Listing: mounting rotation from gravity and a forward push | pass |
| F9-10 | `gyro_heading.cpp` | Listing: heading with and without bias calibration | pass |
| F9-10 | `parked_creep.cpp` | forensic: calibration taken while the robot was moved | pass (expected fault shown) |
| F9-11 | `record_scan.cpp` | simulator that records a scan log ("scan log v1") | pass |
| F9-11 | `view_scan.cpp` + `view_scan.in` | Listing: ASCII top view of a recorded scan | pass |
| F9-11 | `run.sh` → `log_matches_record` | checks `view_scan.in` equals the recording | pass |
| F9-11 | `mount_forensic.cpp` | forensic: wrong mounting offset | pass (expected fault shown) |
| F9-12 | `camera_budget.cpp` | Listing 1: data rate, focal length, blur | pass |
| F9-12 | `depth_error.cpp` | Listing 2: stereo depth error, registration shift | pass |
| F9-12 | `frame_latest.cpp` | Listing 3: keep-latest frame slot (the fix) | pass (age 40.0–66.7 ms) |
| F9-12 | `frame_queue.cpp` | forensic: FIFO queue latency grows | pass (expected fault shown: 1040 ms) |
| F9-13 | `power_budget.cpp` + `.in` | Listing 1: budget with regulator efficiency | pass (81 min estimate) |
| F9-13 | `discharge_log.cpp` | Listing 2 / forensic: cut-off from sag | pass (expected fault shown: cut at 30.3 min) |
| F9-14 | `bus_budget.cpp` + `.in` | Listing 1: bus loads | pass (Ethernet over the 70 % rule, as intended) |
| F9-14 | `can_probe.cpp` | Listing 2: SocketCAN socket | pass in the container (prints the kernel's refusal); **untested on hardware** |
| F9-14 | `can_sched.cpp` (+ `can_model.h`) | forensic: CAN overload starves control | pass (expected fault shown: wheel_cmd 0 sent, 66 timeouts) |
| F9-15 | `plan_check.cpp` + `.in` (+ `plan_rules.h`) | Listing 2: plan checker, revision A | pass (0 errors, 1 warning, as intended) |
| F9-15 | `plan_forensic.cpp` + `.in` | forensic: plan revision C | pass (expected faults shown: 3 errors) |

No listing is an expected compile failure. The optional "Part B" of every lab, which uses kit hardware, is untested on hardware.

### Scratch runs (edited copies, not in the lab folders)

**F9-08**
- `wheel_torque` with slope 15°: OK window 20–40.
- `wheel_torque` with V_low 0.85: OK window 15–30.

**F9-09**
- `odometry` with track 0.165: heading 349.098°, end (−0.0902, 0.1041).
- `odometry` with diameter 0.071: heading 365.151°, end (0.0465, −0.0435).

**F9-10**
- `gyro_heading` with an 8 s bias window: 0.70° error, bias estimate 0.00388.

**F9-11**
- `record_scan` with mountX 0.30: the robot marker moves and the room stays. Used in a Check-yourself answer.
- Figure 2 has 337 points. A small scratch Python script computed them from `view_scan.in`: angle and range for each valid beam, converted to SVG coordinates. It was not kept as a lab file.

**F9-12**
- `frame_queue` with work 30 ms: ages 30.0–31.0 ms, queue 0.
  - The extra 1 ms comes from the simulator's 1 ms step.

**F9-13**
- `discharge_log` with R_int 0.05 Ω: cut at 47.3 min, 2.995 Ah.
- `discharge_log` with R_int 0.03 Ω: cut at 54.3 min, 3.438 Ah.
- `discharge_log` with ramp current 6 A: cut at 46.3 min, 2.624 Ah.
- Figure 2 is plotted from `discharge_log.out` by a scratch script, with coordinates computed linearly from the logged values.

**F9-14**
- `can_sched` with IMU at 100 Hz: bus 25.2 %, all sent, no timeouts.
- `can_sched` with IMU ids 0x400–0x403 at 1 kHz: control messages are sent, but battery gets 0 sent, IMU 1800/1793/1640/1127 of 2000, and the bus is 100 %.

**F9-15**
- `plan_check.in` with encoder_L on 5V: 0 errors, 1 warning, 5V rail 0.64 A.
- `plan_forensic.in` with imu2 at 0x69: 2 errors.

## Unverified boxes and claims that could not be verified, per chapter

Every chapter has two unverified boxes: one on the device values and one on the lab's Part B, which is untested on hardware. Every chapter also has a safety box.

- **F9-08 Actuators**
  - Motor, gearhead, servo, stepper and driver values are all pretend: mass, wheel radius, rolling coefficient, efficiency 0.80, stall torque, no-load speed, stepper torque and pull-out behaviour.
  - Voltage scaling of the brushed DC model is to be checked against the chosen motor.
  - Unverified claims: Hughes & Drury on motor curves and gearing; Lynch & Park on actuation.
- **F9-09 Odometry**
  - Wheel diameter 0.070 m, track width 0.160 m and 1440 counts per revolution are pretend.
  - Part B: the way the kit exposes encoder counts is unknown.
  - Unverified claims: Borenstein & Feng on systematic and non-systematic errors and calibration (title only); Thrun/Burgard/Fox motion models.
- **F9-10 IMU**
  - Readings, bias 0.004 rad/s, noise, 100 Hz rate and axis directions are pretend.
  - Register names, FIFO and scale factors come from the chosen IMU's datasheet.
  - Unverified claims: Woodman's inertial-navigation report; Beard & McLain on frames.
- **F9-11 LiDAR**
  - The simulator's 360 beams at 1°, 0.15–6 m range and ±5 mm noise are pretend.
  - The ROS 2 LaserScan message is named only inside an unverified box.
  - The beam angle convention and the recording format of a real LiDAR are unknown.
  - IEC 60825-1 is title only.
- **F9-12 Cameras**
  - Modes, the 40 MB/s link, 70° FOV, stereo f/B/Δd, the colour offset, 30 fps and the 40 ms detector are pretend.
  - USB Video Class and MIPI CSI-2 are title only.
  - Szeliski and Hartley & Zisserman are title only.
- **F9-13 Power**
  - All currents, efficiency 0.85, 5 Ah, R_int 0.10 Ω, the 10.9 V cut-off with its 0.5 s delay, and the straight-line OCV model are pretend.
  - ISO 13850 is title only. The hardware motor cut is the course rule from F9-07, not a reading of the standard.
  - The Art of Electronics is title only.
  - The worked example's wire resistance (0.013 Ω/m, 15 A) is a pretend wire table.
- **F9-14 Buses**
  - The CAN worst-case frame length 8n + 47 + ⌊(34 + 8n − 1)/4⌋ (135 bits for 8 bytes) is recorded from memory. It must be checked against Davis, Burns, Bril, Lukkien (2007) and ISO 11898-1 / Bosch CAN 2.0.
  - These are pretend or unverified: the UART 6-byte header, the Ethernet overhead of 28 + 38 bytes per packet, the I2C read model, 500 kbit/s, the termination statement, and the CANopen mention.
  - Only the UAPI constants were verified.
- **F9-15 Wiring plan**
  - All parts, levels and addresses are pretend. The `cite=D2…D11` entries are placeholders.
  - The statement on input protection diodes and "VDD + 0.3 V" style limits describes a common pattern. It must be checked in each real datasheet.

## Proposed source-registry additions

- Austin Hughes, Bill Drury, "Electric Motors and Drives: Fundamentals, Types and Applications" (tier 3). It is used by F9-08 and F9-13, and HW302 uses it too. It is not in the guide's registry; please add it after the Source Researcher confirms the edition.
- Robert I. Davis, Alan Burns, Reinder J. Bril, Johan J. Lukkien, "Controller Area Network (CAN) schedulability analysis: Refuted, revisited and revised", Real-Time Systems, 2007 (tier 5). Used by F9-14.
- J. Borenstein, L. Feng, "Measurement and Correction of Systematic Odometry Errors in Mobile Robots", IEEE Transactions on Robotics and Automation (tier 5). Used by F9-09.
- Oliver J. Woodman, "An introduction to inertial navigation", University of Cambridge Computer Laboratory technical report (tier 5). Used by F9-10.

## Proposed analogy mappings (world F9: riding a bicycle; a body with senses and muscles)

Already registered and used: encoder = counting wheel turns; IMU = inner-ear balance sense; sensors and actuators = senses and muscles. Proposed new mappings:

- **Power**
  - Battery = the rider's stored energy (what he ate).
  - Voltage sag at peak current = being unable to push hard on a hill while tired.
  - Protection cut-off = legs refusing to push.
  - Linear regulator = braking downhill (energy turned into heat); switching regulator = changing gear.
  - Used in F9-13.
- **Buses**
  - A shared bus = the team's shared radio channel.
  - CAN arbitration = the more urgent call keeps the channel.
  - Bus load = how much of the time somebody is talking.
  - Termination = soft walls against echoes.
  - Used in F9-14.
- **Datasheets and plans**
  - Datasheet values = the numbers printed on the tyre's sidewall or in the frame's manual.
  - Wiring plan = the club's pre-tour check list.
  - Logic-level compatibility = hand signals big enough for the next rider to read.
  - Used in F9-15.
- **Cameras**
  - Camera = eyes.
  - Latency = pictures that arrive late.
  - Stereo depth = two eyes.
  - Used in F9-12.

## Glossary

- `glossary.json` has 46 four-part entries for the new terms of F9-08 to F9-15.
- Terms that already exist in other courses are linked, not redefined:
  - Actuator (RB101); Dead reckoning (MA301); Pose (2D), Body frame (robot frame) and Rotation matrix (3D) (MA201 and others).
  - Open-drain (wired-AND), I2C, SPI, UART, CAN and Baud rate (HW204).
  - The HW302 sensor, motor and battery terms; Datasheet, Absolute maximum ratings and Schematic (HW303); Fuse (HW101); Star ground (HW302).
- The F9-10 and F9-11 links to "Body frame" now point to MA201's "Body frame (robot frame)". The chapters' jargon boxes still explain the term in their own words.

## Decisions left to the owner

1. **Course kit.** No robot kit, motor, encoder, IMU, LiDAR, camera, battery pack, regulator or CAN interface has been chosen. Every lab's Part B and the course project's real datasheet citations depend on that choice. Per the brief, no commercial kit is named.
2. **Hughes & Drury** in the source registry (see above).
3. **Analogy mappings** for power, buses, datasheets and cameras (see above).
4. **Design rules.** The 70 % bus-load rule (F9-14) and the 80 % rail-load rule (F9-15) are this course's own team rules, stated as such in the text. Confirm them or set other values.
5. **Recorded data format.** F9-11 uses the university's text "scan log v1" instead of a ROS bag, because ROS 2 is not available and is taught in RB303. Decide whether RB201 should switch to bags once RB303's tooling exists.
6. **Forensic scenario for the course card.** "The robot drifts left" is F9-09's forensic lab: wrong wheel diameter parameter versus slip, told apart by whether the encoder count ratio is constant. The other chapters' scenarios were chosen not to repeat HW302's (rolling shutter, brown-out):
   - stepper stall;
   - calibration while moving;
   - mounting offset;
   - FIFO latency;
   - sag cut-off;
   - CAN starvation;
   - logic-level and absolute-maximum violations.
7. **Length.** Most chapters are a little over 5,000 prose words by the build's own count (F9-08 about 5,400), slightly above the L2 range of 3,000–5,000. Each chapter has three listings with line tables plus a full forensic key. Trim if the owner wants the strict range.
8. **`can_probe.cpp`** exits 0 even when there is no CAN support, so that the lab run records the kernel's real answer. On a robot computer it should be extended to bind and read, as described in the lab.
