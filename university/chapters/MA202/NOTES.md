# MA202 Probability and statistics — author and Lab Engineer notes

Author/Lab Engineer run in the cloud build container, 2026-10-09. No internet access; no
book, standard or datasheet was opened. Everything below is for the Source Researcher,
Fact-Checker, Editor, the Dean and the owner. Nothing was committed.

## Files

- Chapters: `F0-56.html` … `F0-63.html`. Each is a fragment per `../FRAGMENT_FORMAT.md`
  with all 21 sections, plus "Answers to Check yourself" with a "Forensic lab answer key" h3.
  Each has two inline SVG figures, claim tags, a Transition box and the `<!--meta` header.
- `glossary.json`: 69 four-part entries, one for every term linked from the chapters'
  "Glossary links" lines.
  - 59 are new.
  - 10 are copied **verbatim** from the course that already defines them, with only
    `chapters` changed to the MA202 chapters. This is so that `render_glossary`, which
    keeps the first course's text, cannot change their definitions. The 10 are:
    - Accelerometer, Bias, Drift (of an integrated gyro), Gyroscope (rate gyro),
      IMU (inertial measurement unit), from HW302.
    - Benchmark, Median and percentile, Run-to-run variation, Warm-up run, from SP302.
    - Sensor noise, from RB101.
    - Pseudo-random number generator, from KID102.
    - Percentage, from MA101.
- Labs: `university/labs/F0-56/` … `university/labs/F0-63/`, 34 sanitizer-built listings
  plus 2 timing programs (`.cc`) built by `F0-63/run.sh`. All of them exit with code 0.

## Sources pattern

- **D-entries** are books and standards cited by title only, with the sentence "not opened
  during this build; the Source Researcher must confirm the edition and section (dossier
  gate G1 open)". The ones used are:
  - Blitzstein & Hwang, "Introduction to Probability": D1 in every chapter except F0-62.
  - ISO/IEC 14882 (the C++ standard).
  - Stroustrup, "A Tour of C++".
  - Horowitz & Hill, "The Art of Electronics".
  - Bryant & O'Hallaron, "Computer Systems: A Programmer's Perspective".
  - Goldberg's floating-point article.
  - IEEE 754.
  - Thrun/Burgard/Fox, "Probabilistic Robotics".
  - Beard & McLain, "Small Unmanned Aircraft".
  - Tukey, "Exploratory Data Analysis".
  - Gregg, "Systems Performance".
- **R-entries** are lab runs in this build. The sanitizer runs used
  `g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0` with
  `-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined`. The F0-63
  timing runs used `-O2` without sanitizers. Every chapter has a `checks.cpp` that
  recomputes every number in the text.
- **W1** is the chapter's own arithmetic. **U1** is the Authoring Guide. **C1** (F0-63 only)
  is curriculum section 13.4, quoted as a map, not a source.
- Random numbers come from the university's own `uniform01`, which is (engine() + 0.5)/2^32,
  and from a Box–Muller Gaussian built on `std::mt19937`. The engine's output sequence is
  fully specified, so every simulation output is reproducible on any conforming library. The
  `std::*_distribution` classes are avoided on purpose because their algorithms are not
  specified. This rests on D-ISO, title only.

## Per chapter

| Chapter | Listings run (all exit 0) | Unverified boxes | Claims resting on title-only sources (G1 open) |
|---|---|---|---|
| F0-56 Chance and counting | dice_count, dice_sim, counting, six_bug (forensic "The certain six"), checks | 1: `std::random_device` / hardware entropy sources | D1 counting rules and the naive definition; ISO C++: mt19937 sequence fully specified, `random_device`; Stroustrup: `<random>` engines, integer types |
| F0-57 Random variables and distributions | coins, uniform_hist, hist_bug (forensic "The histogram that hid the sun"), checks | 0 | D1 PMF/CDF/PDF; Horowitz & Hill: ADC saturation at full scale; Bryant & O'Hallaron: finite representation of numbers |
| F0-58 Mean, variance, standard deviation | stats (+ stats.in), mean_of_n, variance_bug (forensic "The negative wobble"), checks | 1: the Welford attribution and its accuracy | D1 definitions; the name "Bessel's correction" for n − 1; Goldberg / IEEE 754: float spacing, cancellation |
| F0-59 The Gaussian distribution | gauss, clt, alarm_bug (forensic "The fridge that cried wolf"), checks | 1: the accuracy of `std::erf`/`exp`/`log` | D1 Gaussian, CLT, 68–95–99.7; the Box–Muller treatment in D1 (not confirmed that D1 covers it); Thrun: Gaussian beliefs; Horowitz & Hill: physical noise sources; ISO C++: distributions specified by results, not algorithms |
| F0-60 Covariance and correlation | covariance (+ covariance.in), corr_demo, average_bug (forensic "Two thermometers, no improvement"), checks | 0 | D1 covariance, correlation, var(sum); Thrun: covariance matrices in filters |
| F0-61 Bayes' rule | bayes_table, door_filter, alarm_log (forensic "The 98 % camera"), checks | 0 | D1 Bayes, total probability, odds form; Thrun: Bayes filter, door example |
| F0-62 Measurement noise and sensors | imu_sim, imu_stats, noise_avg, gyro_bug (forensic "The robot that turns while standing still"), checks | 3: (a) noise density / bias instability / Allan variance terms; (b) real-IMU sign conventions, axes, scale factor, 9.81 as a simulator setting; (c) **untested on hardware**: the real-IMU variant of the lab | Thrun: measurement models; Beard & McLain: IMU bias, noise, drift; Horowitz & Hill: quantisation Δ/√12, calibration |
| F0-63 Statistics for benchmarks | summary (+ summary.in), test_benchstats (17 tests, 0 failures), claim_check (+ claim_check.in), checks (all sanitizer builds); timing30.cc and capture.cc via run.sh (-O2, no sanitizers) | 2: (a) quartile rules of other tools, and Tukey's 1.5·IQR origin; (b) the build VM's frequency and scheduling, and how run 1's cost divides between filling the data and first touch of pages | Tukey: fences and box plots; Gregg: warm-up, variation, interleaving; D1 quantiles and skew; ISO C++: steady_clock is monotonic |

Expected-fail listings: none. Untested on hardware: the real-IMU variant of the F0-62 lab
only. That chapter has a safety box (adult supervision, low-voltage learner kit, no mains,
no LiPo) and an unverified box that says what a hardware run would need.

## Measured numbers (AH-23)

- **F0-63 timings** come from the build container, a shared cloud VM. The CPU model string
  is in each log; the clocks were not fixed.
  - `timing30.out` and `capture.out` are regenerated by every lab run, so the chapter's
    prose does not quote their values. It describes their shape, and states the range of
    IQR percentages and the cold-run behaviour seen across this build's repeated runs.
  - The forensic evidence `claim_check.in` is a **frozen** copy of one `capture.out`. Its
    first line records the capture date. All forensic numbers in the text (17.511 / 2.640 ms,
    medians 1.7315 / 1.735 ms, and so on) come from that file through `claim_check.cpp`.
    Do not regenerate it unless the text is updated with it.
- **F0-62:** all IMU numbers are simulator settings or results of the seeded simulator.
  No real part's value appears.

## Consistency with other courses

- **Median and nearest-rank percentile:** F0-63 uses exactly the definitions SP302 F2-51
  attributes to it. The course project `benchstats.hpp` is the helper that F2-51 refers to
  ("benchmark-statistics helper"). It sits in `labs/F0-63/` as the reference
  implementation; its tests are `test_benchstats.cpp`.
- **IMU vocabulary:** F0-62 links the HW302 F1-65 glossary entries and states the same
  convention (an accelerometer at rest reads +1 g, upwards). It flags that a real part's
  sign convention must come from its datasheet.
- **Name clash:** F0-62's story uses "Grandpa Tomás", and SP302's analogy world also has a
  "Tomás" (the omelette and stopwatch). The Editor may want to rename one of them.

## Decisions for the owner / Dean

1. **New F0 analogy mappings to register.** Each chapter's `analogy:` meta line lists its
   mappings:
   - dice faces and socks in a drawer (F0-56);
   - the family scoreboard (F0-57);
   - the balance point on a ruler (F0-58);
   - the kitchen thermometer's wobbling last digit (F0-59);
   - sunshine and kitchen temperature (F0-60);
   - the barking dog and the door (F0-61);
   - the kitchen scale with a bias and a calibration bag of rice (F0-62);
   - walking times to school, with the closed level crossing (F0-63).
2. **IMU:** no kit or IMU was chosen, so the F0-62 lab ran on the university's simulator.
   Choose a learner-safe kit and IMU, and record its datasheet as the F0-62 D4 tier-1
   source. A Lab Engineer then needs to do a real run.
3. **Exams:** the course card lists Q, M, F and P exams, including P "analyse a provided
   measurement file". No exam files were produced; that is outside the chapter task.
   `labs/F0-62/imu_rest.csv` and `labs/F0-63/claim_check.in` are ready-made candidates for
   the P exam's measurement file.
4. **Timing builds:** F0-63's timing programs are `.cc` files built by `run.sh` with -O2
   and no sanitizers, following the pattern of SP302 F2-51. Please confirm this exception
   to the sanitizer flags.
5. **Glossary merging:** terms already defined by other courses were copied verbatim
   instead of being redefined. The F0-62 jargon box uses its own analogies for Bias and
   Drift ("10 g" scale, walking with eyes closed), which differ from HW302's glossary
   analogies ("3 grams", Lucía). The precise definitions are identical.
6. `python3 university/build/build.py` rewrites `university/UNIVERSITY.html`, which is a
   tracked file and shows as modified. It reports no PROBLEM line for MA202 or
   F0-56 … F0-63. The remaining PROBLEM lines are broken glossary links in other courses
   whose glossaries were not yet written at build time.
