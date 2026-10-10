# MA201 — Linear algebra for engineers: author notes

Chapters F0-47 to F0-55, L2, 4 credits. Prerequisite: MA101.
Labs are in `university/labs/F0-47` to `university/labs/F0-55`.
Each lab passes `university/labs/run_lab.sh university/labs/<ID>` with exit code 0. All nine were re-run at the end of this build, on 2026-10-09. Every number in the prose, the figures, the worked examples and the answer keys is either printed by a listing or recomputed by that chapter's `checks.cpp`.

## Toolchain and local evidence

- **g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0.** Every `.cpp` is built with `-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined`.
- **No hardware.** This build had no robot, range sensor, camera, IMU or drone. The robot, room, sensor mounts, gyro and drone in F0-53 to F0-55 are simulations written into the listings. Every step that needs a real device is marked "untested on hardware".
- **No GPU listings.** F0-50 previews GEMM (CU302) with CPU code only. It quotes no GPU or CPU performance numbers.
- **No SVG renderer.** The figures could not be rendered in the container (ImageMagick has no SVG delegate). Their coordinates were computed by small Python scripts from the same numbers the listings print. A visual check in a browser is still needed.

## Listings run

No listing is an expected failure: every run exits with code 0. Programs marked "forensic evidence" contain one deliberate bug, and their real output is the evidence pack.

| Chapter | Run | Status |
|---|---|---|
| F0-47 | vectors | pass |
| F0-47 | drive_bug | forensic evidence, exit 0 (subtraction in the wrong order, so the robot drives away from the fridge) |
| F0-47 | checks | pass |
| F0-48 | dot | pass |
| F0-48 | angle_bug | forensic evidence, exit 0 (divides by only one length, giving `nan` for non-unit targets) |
| F0-48 | checks | pass |
| F0-49 | matrix | pass |
| F0-49 | bill_bug | forensic evidence, exit 0 (index `r*rows+c` instead of `r*cols+c`) |
| F0-49 | checks | pass |
| F0-50 | matmul | pass (C++ multiply checked against hand results: the course lab) |
| F0-50 | blocked | pass (tiled result equals naive) |
| F0-50 | gemm_count | pass (operation counts only, no timings) |
| F0-50 | matmul_bug | forensic evidence, exit 0 (accumulator not reset) |
| F0-50 | checks | pass |
| F0-51 | undo | pass |
| F0-51 | inverse_bug | forensic evidence, exit 0 (a and d not swapped) |
| F0-51 | checks | pass |
| F0-52 | rotate2d | pass |
| F0-52 | rotation_util | pass (the exam P item: a 3×3 rotation utility with 10 tests) |
| F0-52 | turntable_bug | forensic evidence, exit 0 (degrees passed as radians) |
| F0-52 | checks | pass |
| F0-53 | sensor_to_world | pass (simulated robot's readings rotated into the world frame: the course lab; every point lies on a wall within 4.4e-16 m) |
| F0-53 | turns_wrong | forensic evidence, exit 0 (the course forensic lab "The robot turns the wrong way": R used where Rᵀ belongs) |
| F0-53 | checks | pass |
| F0-54 | homogeneous | pass |
| F0-54 | chain_bug | forensic evidence, exit 0 ("The cup that follows the robot": T_RS·T_WR instead of T_WR·T_RS) |
| F0-54 | checks | pass |
| F0-55 | quaternion | pass |
| F0-55 | attitude_sim | pass (simulated gyro integration; exits 1 if the attitude is off by more than 1e-9) |
| F0-55 | norm_bug | forensic evidence, exit 0 ("The compass that drifts": first-order update without renormalisation) |
| F0-55 | checks | pass |

**Untested on hardware:** the real-robot repeat of the F0-53 lab, the real sensor mounts in F0-54, and the IMU and drone steps of F0-55. These are deferred to RB301, DN201 and HW302.

**Scratch-only checks:** two quiz answers were checked by running a modified copy of a listing in scratch space. These are not recorded runs, and the chapter says so.
- F0-55 Q8: without the `normalised` call, the final difference is about 5e-15.
- F0-52: the std::apply incident described below.

## Claims resting on books that were not opened (dossier gate G1 open)

Every book is cited by title only, with "not opened during this build". Each chapter's D1 source covers definitions and arithmetic that are shown in the text and recomputed by its runs. The Source Researcher must check D1 against the books listed here.
- **B1, Strang, "Introduction to Linear Algebra"** (all chapters): vectors, dot product, matrices, products, transpose, inverse, determinant, orthogonal matrices.
- **Lynch and Park, "Modern Robotics"** (F0-52 to F0-55, cited as B2): the right-hand rule, rotation matrices, the subscript convention R_sb / T_sb, homogeneous transforms, SE(3), exponential coordinates. The chapters use the subscript reading "T_AB maps B coordinates to A". Check that it matches the book's notation.
- **Beard and McLain, "Small Unmanned Aircraft"** (F0-53, F0-55, cited as B4): aircraft frames, Euler angles, attitude kinematics, the quaternion rate equation.
- **Solà, "Quaternion kinematics for the error-state Kalman filter"** (F0-55, cited as B5): Hamilton and JPL conventions, products, the rotation and matrix formulas. This work is not in registry 4. Decide whether to add it or to replace it with a registry book.
- **ISO/IEC 14882 working draft for C++20** (F0-48, F0-52, cited as B3): `std::atan2`, `<numbers>` (`std::numbers::pi`), `std::hypot` and `std::clamp`.
- **Goto and van de Geijn, "Anatomy of High-Performance Matrix Multiplication"** and **Hwu, Kirk and El Hajj, PMPP** (F0-50): blocking and the GEMM preview.
- **Goldberg, "What Every Computer Scientist Should Know About Floating-Point Arithmetic"** (F0-48, F0-51): rounding examples. All the numbers themselves come from runs.

## Unverified boxes

| Chapter | Box |
|---|---|
| F0-47 | none (pure arithmetic, all run) |
| F0-48 | whether g++ contracts `a*b+c` into a fused multiply-add by default |
| F0-49 | the default storage order of BLAS, cuBLAS, rocBLAS and robotics maths libraries |
| F0-50 | the exact BLAS GEMM argument list and storage assumptions |
| F0-51 | the relative cost of division and multiplication on a given processor |
| F0-52 | the Euler-angle sequence and sign conventions of ROS 2, PX4 and ArduPilot |
| F0-53 | ENU/NED and FLU/FRD frame conventions in real software; untested on hardware (simulated room and sensor) |
| F0-54 | transform naming, storage order and row- or column-vector conventions in libraries (tf2, graphics code); untested on hardware (sensor mounts, gimbals) |
| F0-55 | Hamilton or JPL convention, w first or last, body-to-world or world-to-body (Eigen, ROS 2 messages, PX4, ArduPilot); untested on hardware (no IMU or drone). The safety box says attitude code is never first tried with propellers fitted. |

## Decisions for the owner

1. **New analogy mappings for world F0 (everyday life at home).** Please register or reject each one.
   - Vector: steps across the kitchen floor tiles.
   - Dot product: the shadow of a walk on the direction you face.
   - Matrix: a recipe table (rows are ingredients, columns are dishes).
   - Matrix product: recipe table × weekend plan.
   - Identity, inverse and transpose: "leave it", "undo it" with socks-and-shoes order for two steps, and flipping the recipe card.
   - Rotation: a sheet of squared paper turned about a pin.
   - Coordinate frame: "my left or your left".
   - Transform chain: nested directions, the drawer in the cupboard to the left of the fridge.
   - Quaternion: a skewer twisted in a potato.
2. **Textbook edition.** B1 (Strang) is cited without an edition. One edition should be fixed for the whole maths faculty.
3. **Conventions for the course and the transform library.** These need confirming, because RB301, DN201 and DN301 reuse the library:
   - right-handed frames;
   - x forward and y left in the body frame;
   - angles anticlockwise-positive;
   - R(θ) maps body to world;
   - T_AB maps B coordinates to A;
   - Hamilton quaternions stored (w, x, y, z), mapping body to world;
   - q₂q₁ means "q₁ first";
   - yaw–pitch–roll is R_z·R_y·R_x.
   If DN201 adopts NED/FRD, a conversion chapter or box is needed.
4. **Project reference solution.** The mini-projects build a transform library across F0-47 to F0-55, with rubrics. No reference solution or hidden test `_keys` were written. Decide whether the Lab Engineer should add them.
5. **The std::apply incident (F0-52).** A helper named `apply(const Mat3&, const Vec3&)` failed to compile because argument-dependent lookup found `std::apply`. It was renamed `times`. The real compiler error is quoted in F0-52's "Common mistakes" section, and the failing log was kept in scratch space only. Decide whether this deserves a C++ chapter cross-reference (ADL).
6. **Hand-kept line numbers.** The line numbers in the code walk-through tables are maintained by hand. Any edit to a listing must update its table. A build check that compares them would help.
7. **Forensic labs beyond the card.** The card names one forensic lab ("The robot turns the wrong way", F0-53). The other eight chapters have their own small forensic labs, each with a deliberate-bug program. Review whether they should all be graded.
8. **Exam item P.** The "implement and test a 3×3 rotation utility" item maps to F0-52 `rotation_util.cpp`. The listing is the worked reference, so the exam version should change its rotation axes or its tests.
