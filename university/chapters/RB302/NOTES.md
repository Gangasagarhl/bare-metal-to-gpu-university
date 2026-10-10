# RB302 Estimation: from Bayes to Kalman — author and Lab Engineer notes

The Author and Lab Engineer worked in the cloud build container on 2026-10-10. There was no
internet access, and no book, paper, standard or datasheet was opened. These notes are for
the Source Researcher, Fact-Checker, Editor, the Dean and the owner. Nothing was committed.

## Files

- **Chapters:** `F9-30.html` … `F9-36.html`.
  - Each is a fragment per `../FRAGMENT_FORMAT.md` with all 21 sections, plus "Answers to
    Check yourself" with a "Forensic lab answer key" h3.
  - Each has inline SVG figures (F9-30 has three; the others have two), claim tags, a
    Transition box and the `<!--meta` header.
  - Each fragment was checked before writing:
    - html.parser balance.
    - Every id prefixed with the chapter id; no duplicate ids.
    - No URLs and no `<script>`.
    - Section order.
    - Internal links.
    - Every claim tag has a source, and every source is tagged.
    - Every `data-src` and `data-run` exists, and its run exited 0.
  - `python3 university/build/build.py` reports no PROBLEM line for F9-30..F9-36 or RB302.
    None of the build's broken-link lines points to a link in these chapters.
- **Length:** prose length, excluding listings, tables and SVG, is about 4,400–5,500 words
  per chapter. The L3 target is about 4,000.
- **`glossary.json`:** 40 new four-part entries, built from the chapters' jargon boxes.
  - Terms that other courses already define are linked, not redefined: Bayes filter,
    Prior/likelihood/posterior, Variance, Covariance matrix, Gaussian (normal) distribution,
    Sensor noise, Bias, IMU (inertial measurement unit), Drift (of an integrated gyro),
    Gyroscope (rate gyro), Dead reckoning, Linear approximation and Odometry.
  - Two of these, Dead reckoning and Bayes filter, also appear in a jargon box (F9-30 and
    F9-31) for the reader's convenience. Only the existing glossary entry is published.
  - When this build ran, no other course defined any of the 40 terms.
  - `render_glossary` keeps the first course's text when two courses define the same term.
    If RB301, DN301 or another course written in parallel adds, for example, "Kalman
    filter", "Jacobian" or "Landmark", the Editor should choose one text.
- **Labs:** `university/labs/F9-30/` … `university/labs/F9-36/`, 31 C++ programs and one
  shared header.
  - `mat.hpp` is copied identically into F9-33, F9-34 and F9-35.
  - Every program was built by `run_lab.sh` with
    `g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0` and
    `-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined`.
  - Every program exits 0. There are no expected-fail listings and no GPU listings.
  - Outputs are deterministic: every `.out` was byte-identical on a re-run.
  - Simulators write CSV recordings into their own lab folder (`*_log.csv`, `track_*.csv`).
    Their file names sort before the programs that read them, because `run_lab.sh` builds in
    alphabetical order.
  - **Cross-course dependency:** `F9-35/fuse_ekf.cpp` reads
    `university/labs/F0-62/imu_rest.csv`, the MA202 rest recording. This is how the course
    card's "tune noise covariances from MA202 measurements" is met. If MA202's lab changes
    that file, F9-35's numbers change: re-run F9-35 after any change to F0-62.

## Sources pattern

- **D-entries** are cited by title only, with the sentence "not opened during this build;
  the Source Researcher must confirm the edition and section (dossier gate G1 open)".
  - "Probabilistic Robotics" (Thrun, Burgard, Fox): every chapter.
  - Kalman (1960), "A New Approach to Linear Filtering and Prediction Problems": F9-32 and
    F9-33.
  - Blitzstein & Hwang, "Introduction to Probability".
  - Strang, "Introduction to Linear Algebra".
  - Lynch & Park, "Modern Robotics".
  - Åström & Murray, "Feedback Systems".
  - Beard & McLain, "Small Unmanned Aircraft: Theory and Practice".
  - Mahony, Hamel, Pflimlin, "Nonlinear Complementary Filters on the Special Orthogonal
    Group".
  - ISO/IEC 14882 and IEEE 754.
  - F9-32 also has a tier-1 placeholder (D4) for the course kit's range sensor and encoder
    datasheets, which have not been chosen yet.
- **R-entries** are lab runs in this build. Every chapter has a `checks.cpp` that recomputes
  the worked example and the other quoted numbers.
- **W1** is the chapter's own derivation or arithmetic. **U1** is the Authoring Guide (used in
  F9-30 and F9-31).
- **Random numbers:** `uniform01 = (engine() + 0.5)/2^32` on `std::mt19937`, plus
  Box–Muller, the same pattern as MA202. The `std::*_distribution` classes are avoided
  because their algorithms are not specified.

## Per chapter

All runs listed below pass (exit 0). None is expected-fail. None needs hardware: every robot
is simulated, so every real-robot variant is untested on hardware.

| Chapter | Listings run | Unverified boxes | Title-only claims to check (G1 open) |
|---|---|---|---|
| F9-30 Uncertainty and belief | `belief_grid`, `spread_mc`, `constant_sigma` (forensic "The robot that was always sure"), `checks` | 1: how robot software frameworks (ROS 2, RB303) carry covariance in messages; message names not checked | Probabilistic Robotics: belief definition, the three representations, the Markov assumption; Blitzstein: variance of sums; Strang/ISO only as listed in the chapter |
| F9-31 The Bayes filter | `corridor_bayes`, `zero_likelihood` (forensic "The robot that was 92 % sure it was somewhere else": a likelihood of 0 kills recovery), `checks` | 1: the door detector's 0.9/0.05 and the 0.1/0.8/0.1 motion model are simulator values, not properties of any sensor | Probabilistic Robotics: Bayes filter algorithm, grid localisation, global localisation vs tracking |
| F9-32 A Kalman filter in one dimension (**course lab** and **course forensic** "The overconfident filter") | `cart_sim`, `kf1d` (course lab), `tune_sweep`, `overconfident` (course forensic evidence), `checks` | 2: (a) the names NIS/NEES, chi-square bounds and the whiteness test come from the tracking literature, and no such source is in the registry; (b) untested on hardware, cart/sensor values are simulator settings | Probabilistic Robotics: Kalman filter algorithm and derivation; Kalman (1960): naming and linear formulation; Blitzstein: sums and products of Gaussians |
| F9-33 The Kalman filter in matrix form | `drive_sim`, `kf_cv`, `dt_bug` (forensic "The robot that thinks it is slow"), `dt_fixed` (answer key), `checks` | 1: untested on hardware; statements on single precision and square-root forms not sourced | Probabilistic Robotics: multivariate KF, Joseph form; Strang: matrix operations; Åström & Murray: observability; IEEE 754 rounding |
| F9-34 The extended Kalman filter | `course_sim`, `jacobians`, `ekf_loc`, `wrap_bug` (forensic "The robot that teleports"), `checks` | 1: untested on hardware; real range-bearing sensors are not modelled (correlated, distance-dependent noise) | Probabilistic Robotics: EKF, odometry/landmark models, EKF localisation, data association; Lynch & Park: wheeled kinematics; ISO C++: `atan2` range |
| F9-35 Sensor fusion: odometry and IMU (**course project**, seed of MP6) | `drive_sim`, `fuse_ekf` (reads the MA202 recording), `bias_frozen` (forensic "The robot that slowly turns away"), `checks` | 2: (a) untested on hardware; the claims about bias changing with temperature and power-up are not sourced; (b) the ROS 2 wrapping of the project: `rclcpp`, `nav_msgs/msg/Odometry`, `sensor_msgs/msg/Imu`, `geometry_msgs/msg/PoseWithCovarianceStamped` and the frame conventions were written from memory | Probabilistic Robotics: augmented state, outlier rejection; Beard & McLain: gyro bias models; Mahony et al.: complementary filters |
| F9-36 Particle filters (concept) | `resample_demo`, `pf_corridor`, `deprivation` (forensic "The robot that would not believe it had been moved"), `deprivation_fix` (answer key), `checks` | 2: (a) adaptive random-particle injection and the "Monte Carlo localisation" naming, from memory; (b) untested on hardware; ready-made localisation components of robot stacks not named or checked | Probabilistic Robotics: particle filter, resampling, MCL, kidnapped robot; Blitzstein: sampling, law of large numbers |

### Course-card items and where they are

- **Course lab** ("Kalman filter on recorded data with ground truth; tune noise covariances
  from MA202 measurements"): F9-32.
  - R is measured from a rest recording by the F0-62 method, and Q from ground truth.
  - `tune_sweep` checks the tuning.
  - F9-35 repeats the idea with the actual MA202 file `imu_rest.csv`.
- **Course forensic** ("The overconfident filter": innovation statistics show the measurement
  noise set too small): F9-32. The evidence is real output of `overconfident.cpp`.
  - R is 110 times too small.
  - Mean NIS is 23.1, with 129 of 190 innovations outside ±2√S.
  - Lag-1 autocorrelation is −0.36, against +0.05 for the honest release.
  - The key uses the `tune_sweep` table to separate "R too small" from "Q too small".
- **Course project** (an EKF localisation node, seed of MP6): specified in F9-35's
  mini-project. The ROS 2 wrapping sits inside an unverified box.
- **Exam P** ("implement a filter for a provided dataset"): the CSV recordings in the lab
  folders are suitable, and so are new seeds of the simulators. No exam was written; the
  task did not ask for one.

## Things the Fact-Checker should look at first

1. **F9-32 hardware note:** "any microcontroller with floating-point arithmetic can run it" is
   an operation count (W1), not a measurement.
2. **F9-33 Layer 3:** the continuous-white-noise Q (entries Δt³/3, Δt²/2, Δt) is from memory
   and is not used in any run.
3. **F9-34 Layer 3:** E[cos θ] = cos θ̂·e^(−σ²/2) for a Gaussian θ. `checks.cpp` evaluates
   the formula but does not derive it. It rests on D3 (Blitzstein), title only.
4. **F9-35 Layer 3:** "fixed-gain filters built on this split are called complementary
   filters". The registry's Mahony paper is about attitude on SO(3); its scope should be
   confirmed before it is cited for the planar case.
5. **Answers marked "not run in this build"** (several "Predict, then run" questions and lab
   steps 3–6) give the expected direction of the effect, not numbers. This is deliberate.
   The learner runs them.

## Decisions for the owner

1. **Course kit sensors:** a range sensor for the F9-32 cart, a landmark sensor (camera
   markers or a scanner) for F9-34, and the IMU and encoders for F9-35. Until they are chosen,
   every number is a simulator setting, and each chapter says so in an untested-on-hardware
   box.
2. **NIS/NEES source:** the registry has no target-tracking textbook. F9-32 to F9-35 rely
   on NIS, NEES, chi-square gates and innovation whiteness. Please add one to the registry,
   or accept these as W1 derivations.
3. **Cross-course data file:** F9-35 reads MA202's `imu_rest.csv` in place, through
   `../F0-62/`. The alternative is a copy in F9-35's folder, which is more robust but
   duplicates data. The current choice matches the course card's wording.
4. **Length:** the chapters run 10–35 % above the L3 guide length. The Editor may shorten
   Layer 3 sections. No numbers depend on them.
5. **MP6 interface:** the project's class interface (`onOdometry`, `onGyro`, `onLandmark`,
   `pose`) is a proposal. MP6's authors should confirm it, or replace it, before RB303 and
   MP6 build on it.

## Analogy mappings (proposals for the registry)

The registered mappings used are:

- Kalman filter: "combining two friends' guesses, trusting the more reliable one more".
- IMU: "the balance sense in your inner ear".
- Encoder: "counting the turns of a wheel".

New F9-world mappings proposed by these chapters:

| Concept | Proposed mapping | Chapter | Where it breaks (stated in the chapter) |
|---|---|---|---|
| Belief / uncertainty | How sure you are where you are when riding home at dusk ("somewhere around the boathouse, give or take"); prediction = counting how long you have pedalled; landmark = the lit bakery window | F9-30 | A belief is a whole distribution, not a single interval |
| Bayes filter predict/update | Prediction = "I have pedalled about one more minute"; update = "that lit window must be the bakery" | F9-31 | A person reasons; the filter only multiplies and normalises |
| Hidden-state estimation (velocity from positions) | Judging a bicycle's speed from kilometre posts without a speedometer | F9-33 | The filter never divides distance by time; the link is the covariance |
| Linearisation (EKF) | "A bend in the road looks straight for the next few metres" | F9-34 | "A few metres" shrinks fast; a lost robot has no "nearby" |
| Gyro bias | A slight constant lean in the inner-ear balance sense | F9-35 | The real inner ear adapts; a gyro does not (stated as a picture, not physiology) |
| Wheel slip | A tyre spinning on gravel | F9-35 | The filter notices slip only through innovations |
| Particle filter | A cycling club spread along a ring road, copying the riders whose guess fits | F9-36 | Riders think; particles only move and get weighted |

## Not done in this build

- No real-hardware run of any kind (none was available).
- No exam questions (Q, M, F, P) were written; the task asked only for chapters and labs.
- The ROS 2 node wrapping of the course project was not built (unverified box in F9-35).
