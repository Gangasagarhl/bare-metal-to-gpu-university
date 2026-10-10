# RB301 — Kinematics and dynamics: author notes

Chapters F9-23 to F9-29, L3, 4 credits. Prerequisites: MA201, RB201.
Labs are in `university/labs/F9-23` to `university/labs/F9-29`.
Each lab passes `university/labs/run_lab.sh university/labs/<ID>` with exit code 0. All seven were re-run at the end of this build, on 2026-10-10. Every number in the prose, the figures, the worked examples and the answer keys is either printed by a listing or recomputed by that chapter's `checks.cpp`. The exceptions are the few "predict, then run" answers checked with modified copies, listed below.

## Toolchain and local evidence

- **g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0.** Every `.cpp` is built with `-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined`. Sources are formatted with clang-format (LLVM base, 4-space indent, 100 columns).
- **No hardware.** This build had no robot, arm, camera, tracker or encoders. Every robot in RB301 is the university's own simulated course robot, written into the listings:
  - a differential-drive base: wheel radius 0.05 m, track width 0.30 m;
  - a three-joint arm: yaw, then two pitch joints with axis (0, −1, 0), shoulder 0.10 m above the arm base, links 0.30 m and 0.30 m, mounted at (0.1, 0, 0.3) on the base;
  - a planar 2R arm for F9-26 to F9-28: L1 = 0.30 m, L2 = 0.25 m, and in F9-28 point masses of 1.0 kg and 0.5 kg.

  Every step that needs a real device is marked "untested on hardware".
- **No ROS 2, no MoveIt 2, no URDF parser, no physics simulator** were installed or opened. As the brief requires, no commercial simulator is named. The "simulators" and "trackers" in the labs are our own C++ code.
- **No GPU listings. No timings.**
- **No SVG renderer.** The figure coordinates were computed by hand or by small Python scripts from the same numbers the listings print. A visual check in a browser is still needed, especially for:
  - the F9-24 ICR figure;
  - the F9-29 tree figure, which has text placed close to its arrows.

## Listings run

No listing is an expected failure. Every run exits with code 0. Programs marked "forensic evidence" contain one deliberate fault, and their real output is the evidence pack.

| Chapter | Run | Status |
|---|---|---|
| F9-23 | frame_tree | pass (frame tree world → base → camera/shoulder; cup in four frames) |
| F9-23 | tf_time | pass (stale versus time-interpolated base pose: 0.0500 m versus 0 error) |
| F9-23 | cup_miss | forensic evidence, exit 0 ("The arm misses the cup": camera mount stored as `inverse(T_BC)`, line 17) |
| F9-23 | checks | pass |
| F9-24 | odometry_vs_truth | pass (Euler versus exact-arc odometry against simulator ground truth at 100 Hz and 50 Hz) |
| F9-24 | square_bug | forensic evidence, exit 0 (configured track width 0.30 m, true 0.34 m) |
| F9-24 | checks | pass |
| F9-25 | fk | pass (chain versus closed form, 9,261 configurations, max difference 2.22e-16 m) |
| F9-25 | poe | pass (product of exponentials versus chain, 2,197 configurations) |
| F9-25 | elbow_flip | forensic evidence, exit 0 (elbow axis sign flipped, line 12) |
| F9-25 | checks | pass |
| F9-26 | ik2r | pass (analytic IK, both branches, round trip through FK) |
| F9-26 | ik_numeric | pass (damped least squares IK, converges in 13 iterations) |
| F9-26 | reach_nan | forensic evidence, exit 0 (out-of-reach target gives `acos` of a value above 1, so NaN) |
| F9-26 | checks | pass |
| F9-27 | jacobian | pass (analytic versus finite differences 1.00e-10; det J near singularity; statics) |
| F9-27 | resolved_rate | pass (straight line, 3 s, final error about 1.4e-4 m) |
| F9-27 | whip | forensic evidence, exit 0 (path ends at 0.5496 m of a 0.55 m reach; joint speeds exceed 120 deg/s) |
| F9-27 | checks | pass (includes the damped-least-squares comparison used in the answer key) |
| F9-28 | dynamics | pass (ID/FD round trip 4.80e-14; gravity hold; RK4 energy drift 8.72e-10 J) |
| F9-28 | euler_energy | forensic evidence, exit 0 (explicit Euler at 5 ms gains 5.23 J in 10 s) |
| F9-28 | checks | pass (integrator comparison: explicit Euler, semi-implicit Euler and RK4 at two step sizes) |
| F9-29 | urdf_fk | pass (course_robot.urdf read by our own reader; 5,415 configurations versus closed form, 1.11e-16 m; exits 1 above 1e-12) |
| F9-29 | arm_side | forensic evidence, exit 0 (`rpy="0 0 90"`: degrees written where radians are expected) |
| F9-29 | checks | pass |

**Untested on hardware:** every optional real-robot step in every lab (F9-23 to F9-29), and the hardware sections of all seven chapters.

**Checked only with modified copies in scratch space.** These are not recorded runs, and each answer says it was checked by running a modified listing:
- F9-24 Q7: arc odometry error of about 1e-9 m at every rate.
- F9-25 Q6: tool offset changed, sweep reports 1.000e-2 m and exit 1.
- F9-26 Q7: maxStep = 100 converges after 24 iterations to (−309.806°, 432.021°, −798.788°).
- F9-27 Q7: kp = 0 ends about 4e-4 m off.
- F9-29 Q7: elbow axis set to (0, 1, 0), sweep reports 5.91e-01 m and exit 1.

## Claims resting on sources that were not opened (dossier gate G1 open)

Books are cited by title only, with "not opened during this build". Each chapter's D1 covers definitions and arithmetic that are shown in the text and recomputed by its runs.

- **B1, all chapters:** Lynch and Park, "Modern Robotics" (registry 4.7). These chapters rest on it:
  - rigid transforms (F9-23);
  - wheeled-robot kinematics (F9-24);
  - forward kinematics, PoE and DH (F9-25);
  - inverse kinematics (F9-26);
  - velocity kinematics and statics (F9-27);
  - open-chain dynamics (F9-28);
  - inertia (F9-29).

  The Source Researcher must confirm the edition and the section numbers.
- **B2, F9-24:** Thrun, Burgard and Fox, "Probabilistic Robotics", cited for odometry.
- **S1, F9-23:** ROS 2 documentation for tf2. Nothing was opened.
- **S1, F9-26:** ISO/IEC 14882, cited for the ranges of `atan2` and `acos`. The behaviour was observed in runs, but the standard was not opened.
- **S1 and S2, F9-29:** the URDF specification, the ROS 2 robot-description documentation and the SDF specification. Nothing was opened.
- **U1, all chapters:** the University Authoring Guide course cards. Cited only for what other courses teach.

## Unverified boxes, per chapter

Every chapter has two boxes: one on software or specifications, and one "untested on hardware".

- **F9-23.** (1) tf2 in ROS 2:
  - the static versus dynamic broadcasters;
  - the lookup-transform call, its argument order (target, source) and its time handling;
  - the frame names `map`/`odom`/`base_link`;
  - the camera optical-frame convention.

  (2) Untested on hardware: the camera optical frame and the calibration procedure.
- **F9-24.** (1) The ROS 2 names: the `odom → base_link` transform, the odometry and twist messages, and the differential-drive controller of ros2_control. (2) Untested on hardware: encoder counts, gear ratios, real slip.
- **F9-25.** (1) The screw-axis/PoE notation and the DH variants, written from the author's recollection of "Modern Robotics". The runs prove the formulas as implemented agree with the chain; they do not prove they match the book's notation. (2) Untested on hardware: real link lengths, axis directions and joint zeros.
- **F9-26.** (1) MoveIt 2 IK solver plugins and their settings. (2) Untested on hardware: joint limits and real workspace.
- **F9-27.** (1) The servoing component of MoveIt 2 and its singularity thresholds. (2) Untested on hardware: the 120 deg/s limit and the 10 N load are simulated values.
- **F9-28.** (1) The integrators and step settings of physics simulators used with ROS 2, and ROS 2 gravity-compensation/feed-forward interfaces. (2) Untested on hardware: torque mode, torque constant, friction.
- **F9-29.** (1) All URDF element and attribute names were written from memory. These conventions are also unverified:
  - rpy in radians, composed as Rz·Ry·Rx;
  - the joint frame equal to the child frame;
  - whether `limit` also needs `effort` and `velocity`.

  SDF is described conceptually only, and the ROS 2 tool names are unverified. `course_robot.urdf` has been read only by our own `urdf_mini.hpp`. (2) Untested on hardware: every dimension and mass in the model file.

## Analogy mappings proposed (F9 world: riding a bicycle; a body with senses and muscles)

None of these is registered yet; please accept or replace them.

| Chapter | Proposed mapping |
|---|---|
| F9-23 | eyes = camera frame; neck = camera mount (static transform); shoulder = arm base frame; "where is the cup relative to my shoulder, given where my eyes see it" = a frame-tree lookup |
| F9-24 | pushing the two hand-rims of a wheelchair = commanding the two wheels; both rims equally = straight; one rim only = turning about the other wheel |
| F9-25 | shoulder and elbow = joints; upper arm and forearm = links; "touch your nose with your eyes closed" = forward kinematics from felt joint angles |
| F9-26 | reaching for a cup with the elbow up or down = the two IK branches; a cup beyond your fingertips = out of the workspace |
| F9-27 | wiping a whiteboard: how far the hand moves for a small twitch of each joint = the Jacobian's columns; a fully stretched arm that cannot reach farther = a singularity; locking the elbow to push a door = τ = JᵀF at a singularity |
| F9-28 | a shopping bag held at arm's length = gravity torque; a swung bag that is hard to start and stop = inertia; a playground swing never goes higher than where it was let go = the energy check |
| F9-29 | a bicycle's parts list with how each part is bolted or hinged to the next = a robot description file |

The story hooks use the people Mei (F9-27), Tomás (F9-28) and Ana (F9-29), and similar names elsewhere. If the owner keeps a cast list, they may need renaming.

## Glossary

`glossary.json` holds 45 four-part entries, generated from the chapters' jargon boxes, so the wording is identical. Terms already defined by other courses are linked, not redefined:
- MA201: Coordinate frame, Homogeneous transform, Direction vector;
- RB101: Differential drive, Encoder;
- MA301: Dead reckoning, Derivative;
- RB201: Torque.

Two RB301 terms deliberately differ from existing ones, because build.py merges entries by exact term and the first definition wins:
- "Frame tree (transform tree)" differs from MA201's "Frame tree".
- "Pose (3D)" differs from MA201's "Pose (2D)".

## Decisions for the owner

1. **ROS 2 / tf2 / MoveIt 2 / URDF / SDF verification.** The course card asks for tf2 and URDF. This build could only teach the concepts with our own code and put every name in unverified boxes. Before RB401 relies on RB301, someone with the specifications must:
   - check the F9-23 tf2 list and the F9-29 URDF list;
   - load `course_robot.urdf` with a standard parser, and add `effort`/`velocity` attributes if the parser requires them.
2. **The course robot's dimensions** are simulator values chosen by this author: wheels r = 0.05 m and W = 0.30 m; arm links 0.30/0.30 m; planar arm 0.30/0.25 m; masses. They should be replaced by the kit's measured values once the kit is fixed in the dossier. The labs are written so that this is a one-file change per lab.
3. **The course card says "simulate; verify kinematics against simulator ground truth".** Because no commercial simulator may be named or was available, the "simulator" is our own C++ code. In F9-29, the model is verified against a closed form and against a simulated tracker. If the owner wants a physics simulator in this lab, it belongs to RB401 or needs a dossier decision.
4. **RB201 chapters F9-08 to F9-15 were not present** when these chapters were written; only their labs were. Cross-references to RB201 (motors, torque, encoders) and RB202 (control) are by chapter id only and may need wording changes when those chapters land.
5. **Maths prerequisites.** F9-27 and F9-28 use derivatives, differential equations and integrators. MA301 and MA302 are not formal prerequisites of RB301, so each of those chapters has a "maths you need here" box. Consider adding MA301 as a recommended prerequisite on the course card.
6. **Word counts** run above the L3 target of about 4,000 words: about 5,500 to 6,600 words of non-code text per chapter, including tables and answer keys. The owner may want the Layer 3 sections trimmed.
7. **The course project** (a kinematics library reused by RB401) is specified across the chapters' mini-projects:
   - `kin/frames`, `kin/diffdrive`, `kin/fk`, `kin/ik`;
   - `kin/jacobian`, `kin/dynamics`, `kin/model`.

   No reference solution is shipped; the lab headers (`frames.hpp`, `diffdrive.hpp`, `arm.hpp`, `ik.hpp`, `jac.hpp`, `dyn.hpp`, `urdf_mini.hpp`) are a starting point.
8. **Exams (Q; M; F; P: compute and verify an arm's forward kinematics).** No separate exam files were written; the F9-25 lab and the F9-29 sweep test are natural P items.
