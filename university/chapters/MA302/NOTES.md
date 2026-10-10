# MA302 Numerical methods and floating point — author notes

Author / Lab Engineer run, build of 2026-10-09. No internet; no source document was opened. Every book or spec is cited "title only — not opened during this build (dossier gate G1 open)".
Level L3, faculty F0 (analogy world: everyday life at home). Card goals: IEEE 754 representation and rounding; choosing and justifying test tolerances; integrating equations of motion; solving linear systems; discretising a controller; FP16/BF16/FP8 accuracy.
Card labs and where they are:
- "Sum 2^20 floats in different orders": F0-75 Listing 1 (`orders.cpp`).
- "Integrate a pendulum with two methods": F0-76 Listing 1 (`pendulum.cpp`).
- "Set the tolerance for a reduction-kernel test": F0-80 Listing 4 (`reduce_test.cpp`).
Forensic lab of the card, "The test that fails on Tuesdays": F0-75 (`tuesday.cpp`).
Maps: maths rung 8; curriculum E3 (Kahan-summed reference, exact integer sums) and E7 (tolerance justified from FP16/BF16 input rounding). The E3 and E7 acceptance sentences are quoted verbatim in F0-80 Layer 2.
Project: the HTML numerics notebook. Each chapter's mini-project adds one page, and F0-80's mini-project assembles them.

## Files

- Chapters: `F0-73.html` … `F0-80.html`. Each has the 21 template sections in order, plus "Answers to Check yourself" with the forensic answer key. Each also has two inline SVG figures (F0-79 fig. 1 is the required sampled control loop: sampler, controller, zero-order hold, plant), claim tags on every factual sentence, a Transition box, and at least one "Not verified" box.
- Every fragment was checked with an assembler/validator script (scratch, not committed):
  - tag balance (html.parser);
  - ids prefixed with the chapter id, and no duplicate ids;
  - no URLs, no `<script>` or `<style>`;
  - section order;
  - no dangling `#F0-…` links;
  - every `data-src` and `data-run` file exists;
  - every defined source is cited.
- Approximate prose word counts: 6,060 / 5,810 / 5,700 / 5,400 / 5,430 / 4,990 / 5,550 / 6,290.
- The banned words ("typically", "simply", "just") were checked: none.
- Glossary: `glossary.json` has 46 four-part entries (simple, analogy, precise, source) plus related and chapters. All are plain text, generated from the chapters' Jargon boxes so the two cannot disagree.
- Terms already defined elsewhere are **linked, not redefined**:
  - "Catastrophic cancellation" (MA202) and "Truncation error" (MA301). F0-74's Jargon box explains both in-chapter, consistent with those entries; the glossary entries are theirs.
  - FP16 / BF16 and Mixed precision (HW301).
  - FMA, Reduction and Floating-point reassociation (SP302).
  - Floating-point tolerance (SP101).
  - From MA301: Sample time (Δt), Trapezoid rule, PID controller (discrete), Pole (of a transform), Simulation step (Euler step), State (of a dynamic system).
- `build.py` run: no PROBLEM line mentions MA302, F0-73…F0-80 or any `gl-` term they link. The 45 remaining PROBLEM lines are other courses' glossary links.

## Lab conventions

- Every folder `university/labs/F0-73` … `F0-80` passes `university/labs/run_lab.sh university/labs/<ID>` from the repo root (all eight re-run at the end of the build, exit 0).
- `.cpp` files are built by `run_lab.sh` with `g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined`.
- `run.sh` (F0-73, F0-75, F0-80) adds steps that need other flags or tools. Each step writes `.out` and a `.log` with listing, toolchain, command, date, machine, optional hardware note, and exit code.
- `F0-80/stdfloat_check.cc` is a `.cc` file on purpose. It needs `-std=c++23` (`<stdfloat>`), so `run_lab.sh` must skip it, and `run.sh` builds it.
- `.timeout` files raise the limit for the long sanitized runs: F0-75 tuesday*, F0-80 always_passes and reduce_test.
- Shared headers are copied, not shared across folders, so each folder runs on its own:
  - `F0-78/solve.hpp` is a copy of `F0-77/solve.hpp`; keep them identical.
  - `F0-80/lowp.hpp`'s splitmix64 matches `F0-75/data.hpp`.
- Toolchains: `g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0`; `Cuda compilation tools, release 12.0, V12.0.140`; objdump from binutils; Linux x86_64 build container. **No GPU.**

## Listings run

| Chapter | Listings and evidence | Result |
|---|---|---|
| F0-73 | `bits.cpp` (L1), `spacing.cpp` (L2), `encode.cpp` (worked), `counter.cpp` (forensic), `addss.cpp`; run.sh: `disasm` (objdump: addss/addsd), `nvcc_ftz_help` | pass, exit 0 |
| F0-74 | `roundoff.cpp` (L1), `cancel.cpp` (L2), `steps.cpp`, `evalmethod.cpp`, `drift.cpp` (forensic) | pass, exit 0 |
| F0-75 | `orders.cpp` (L1, course lab), `kahan.cpp` (L2), `tuesday.cpp` (course forensic), `tuesday_fix.cpp`; run.sh: `kahan_fastmath` (-O2 vs -O2 -ffast-math) | pass, exit 0 |
| F0-76 | `pendulum.cpp` (L1, course lab), `order.cpp` (L2), `worked.cpp`, `swing.cpp` (forensic), `swing_fix.cpp` | pass, exit 0 |
| F0-77 | `gauss.cpp` (L1), `pivot.cpp` (L2), `calib.cpp` (forensic), `calib_fix.cpp` | pass, exit 0 |
| F0-78 | `hilbert.cpp` (L1), `stable.cpp` (L2), `worked.cpp`, `beacons.cpp` (forensic), `beacons_key.cpp` | pass, exit 0 |
| F0-79 | `pi_discrete.cpp` (L1), `poles.cpp` (L2), `closedloop.cpp` (L3), `velocity.cpp` (worked), `buzz.cpp` (forensic), `buzz_fix.cpp` | pass, exit 0 |
| F0-80 | `formats.cpp` (L1), `dot.cpp` (L2), `reduce_test.cpp` (L4, course lab), `worked.cpp`, `always_passes.cpp` (forensic); run.sh: `stdfloat_check` (0 differences from std::float16_t / std::bfloat16_t over 200,000 values), `fmad_ptx`, `nvcc_fmad_help`, `reduce_ptx` | pass, exit 0 |
| F0-80 | `reduce.cu` (L3) | **untested on hardware**: built with nvcc; the run exits 1 with the runtime's own "no CUDA-capable device is detected" (the expected result in this container; `reduce.log` carries the hardware note) |

Every forensic evidence program contains its deliberate fault as described in the answer key. Every number quoted in prose comes from the committed `.out` files. All programs are deterministic (seeded splitmix64, fixed data), so re-running reproduces them.

## Unverified claims (all in "Not verified" boxes)

- **F0-73.** GPU rounding and subnormal behaviour per instruction, and fast intrinsics: CUDA Programming Guide, AMD HIP and ISA docs. The x86 rounding-mode control register (MXCSR): Intel SDM.
- **F0-74.**
  - The Higham–Mary probabilistic rounding error analysis (title, year, constants).
  - x87 double rounding, GCC's default `-ffp-contract` in ISO mode, and which x86-64 baseline lacks FMA. All from memory; check in the GCC 13 manual and Intel SDM.
- **F0-75.**
  - GPU float atomics complete in unspecified order.
  - The names of deterministic or reproducible reduction routines (CUB, rocPRIM).
  - The GPU sums are untested on hardware: they come from a CPU model of the kernel's order.
- **F0-76.** No physical pendulum or robot. The integrators used by named physics engines and flight stacks are not stated.
- **F0-77.** LAPACK `dgesv`, Eigen `PartialPivLU`, cuBLAS/cuSOLVER batched routines: names and behaviour. No arm or MCU; the calibration data are invented.
- **F0-78.** The "dilution of precision" link and LAPACK condition estimators. The ±0.2° bearing error is a scenario assumption.
- **F0-79.**
  - MCU timer, ADC and PWM details.
  - Loop rates of PX4, ArduPilot and ros2_control (deliberately not stated).
  - All plants are models with lab numbers. There is also a Safety box for the optional RB202 motor kit.
- **F0-80.**
  - **The OCP FP8 E4M3 definition.** It is reported to have no infinities, a single NaN pattern and max 448, with saturating conversions; this course's `kE4M3` model is IEEE-like with max 240.
  - TF32.
  - The Intel BFLOAT16 white paper.
  - Which input and accumulator precisions the matrix units support, and their internal rounding.
  - `reduce.cu` is untested on hardware.

## Analogy proposals (F0 world, for the analogy registry)

| Concept | Proposed analogy | Chapter |
|---|---|---|
| Floating-point number | a measuring set of scoops, each twice the size of the last, with the same marks on every scoop | F0-73 |
| Rounding error / its growth | the spill each time a scoop is levelled; many spills added | F0-74 |
| Kahan summation | Amara's notepad of the crumbs each levelled scoop leaves, added back with the next | F0-75 |
| Integration step / RK4 | walking to the kitchen in the dark, one step in the facing direction / peeking several times before committing | F0-76 |
| Linear system / pivot | prices from several shopping receipts / never divide by a smudged near-zero amount | F0-77 |
| Conditioning / stability | a wobbly table / a careless way of carrying a full cup | F0-78 |
| Sampled controller | adjusting the bath tap but only looking at the thermometer every so often | F0-79 |
| Reduced precision / FP32 accumulation | coarse scoops / the running total written in a notebook with many digits | F0-80 |

Characters used: Amara, Kenji and Laila only.

## Decisions for the owner / Dean

1. **Textbooks (registry 4.2).** Proposed:
   - Higham, "Accuracy and Stability of Numerical Algorithms", as the MA302 core text (B1 in most chapters);
   - Trefethen & Bau, "Numerical Linear Algebra", for F0-77 and F0-78;
   - Hairer–Nørsett–Wanner, "Solving ODEs I", and Hairer–Lubich–Wanner, "Geometric Numerical Integration", for F0-76;
   - Åström & Wittenmark, "Computer-Controlled Systems", for F0-79 (Åström & Murray is already registered).
   Editions and sections must be recorded when the dossier opens G1.
2. **FP8 source of record.** OCP OFP8 (S2 of F0-80) is proposed, with the Micikevicius et al. paper as background.
   - Once checked, `kE4M3` in `F0-80/lowp.hpp` should probably follow OFP8 (max 448, no infinities). That changes the E4M3 rows of `formats.out` and `dot.out` and the figure. The chapter flags the model explicitly.
3. **C++23 cross-check step.** `stdfloat_check.cc` needs `-std=c++23`, outside the runner's C++20 contract, so it runs from `run.sh`. Keep it this way, or extend `run_lab.sh` to honour a per-file standard.
4. **CPU model of GPU order.** F0-75 and F0-80 test a CPU model of the kernel's addition order instead of the GPU. The PTX (run R9) shows only `add.f32`, which supports the model, but bitwise equality on hardware is an open prediction. It should be run on a GPU when the faculty has one (F0-80 lab step 5).
5. **E3 size.** E3 says 2^28 floats; the course labs use up to 2^22 (sanitized CPU runs). The tolerance formula scales, and the worked answer shows how. A 2^28 run belongs on the GPU in Track E.
6. **Glossary overlaps.**
   - MA202 defines "Catastrophic cancellation" and MA301 "Truncation error". MA302 links to them; MA302 does not redefine them.
   - MA301's "Sample time (Δt)" and MA302's "Sample period (sampling rate)" are near-synonyms. Merge them if the Dean prefers one term.
