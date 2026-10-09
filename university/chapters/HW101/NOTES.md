# HW101 Electricity and circuits for engineers — author notes

Author / Lab Engineer run, build of 2026-10-09 (no internet; batch brief `AUTHOR_BRIEF.md` + `AUTHOR_BRIEF_UPPER.md`).
Level L1, faculty F1 (analogy world: the restaurant building and its kitchen).
Card: "Maps to: Prepares HW102, HW302" — HW101 has **no curriculum milestone**, so no acceptance tests
were copied from `SYSTEMS_CURRICULUM.html`; the labs reuse the course card's lab wording verbatim
("Simulator then low-voltage breadboard: LED current limiting, RC timing measured against prediction")
and the course forensic lab "The hot resistor" (F1-03). No bridge chapter of guide 6.2 touches HW101.

## Files

- Chapters: `F1-01.html` … `F1-08.html` (all 21 sections + answers, forensic answer key inside Answers).
- Glossary: `glossary.json` — 52 four-part entries, generated from the chapters' Jargon boxes so the two
  never disagree; every `#gl-…` link in the chapters resolves to one of them.
- Labs: `university/labs/F1-01` … `F1-08` (sources, `.in`, `.out`, `.log`).

## Listings run (all with `university/labs/run_lab.sh`, from the repository root)

Toolchain: `g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0`,
`-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined`, Linux x86_64 build container.

| Chapter | Listing (chapter use) | Result |
|---|---|---|
| F1-01 | `charge.cpp` + `charge.in` (Listing 1), `probes.cpp` + `probes.in` (forensic evidence) | pass, exit 0 |
| F1-02 | `ohm_fit.cpp` + `ohm_fit.in` (Listing 1), `bench.cpp` + `bench.in` (forensic) | pass, exit 0 |
| F1-03 | `power.cpp` + `power.in` (Listing 1), `hot_resistor.cpp` + `hot_resistor.in` (course forensic "The hot resistor") | pass, exit 0 |
| F1-04 | `network.cpp` + `network.in` (Listing 1), `divider_load.cpp` + `divider_load.in` (forensic) | pass, exit 0 |
| F1-05 | `rc_charge.cpp` (Listing 1), `rc_coarse.cpp` (answer to quiz Q8), `blink_log.cpp` + `blink_log.in` (forensic) | pass, exit 0 |
| F1-06 | `led_model.cpp` (Listing 1), `two_leds.cpp` + `two_leds.in` (forensic) | pass, exit 0 |
| F1-07 | `meter_effect.cpp` (Listing 1), `meter_log.cpp` + `meter_log.in` (forensic) | pass, exit 0 |
| F1-08 | `digitize.cpp` (Listing 1), `relaxation.cpp` (Listing 2, course-project predictor), `bounce.cpp` + `bounce.in` (forensic), `bounce_fixed.cpp` + `bounce_fixed.in` (forensic answer key) | pass, exit 0 |

No expected-fail listings. No CUDA/HIP, so nothing is "untested on hardware" in the GPU sense; every
**physical** (breadboard/kit) lab part is marked *not performed in this build* in an unverified box.
All forensic evidence packs are program output from real runs (never typed by hand); each answer key
says how the evidence was generated.

Two ad-hoc checks were run but not kept as listings: F1-06 troubleshooting ("0 Ω gives inf / -nan" —
said so in the chapter), and a coarse-step check for F1-05 that was then kept as `rc_coarse.cpp`.

## Claims that could not be verified (sources title only, gate G1 open)

No source document could be opened (no internet). Every technical claim is tagged to one of:

- D1 Charles Platt, "Make: Electronics" (title only).
- D2 Paul Horowitz and Winfield Hill, "The Art of Electronics" (title only) — the main source for
  physics and circuit claims (potential and ground, Kirchhoff's laws, dividers and loading, Thévenin,
  RC law, diode equation and breakdown, load lines, current hogging, meter loading/burden, logic levels,
  noise margin, Schmitt triggers, relaxation oscillators, sampling/quantisation).
- D3 BIPM, "The International System of Units (SI)" (title only) for unit relations
  (1 A = 1 C/s, 1 V = 1 J/C, 1 Ω = 1 V/A, 1 W = 1 J/s, 1 F = 1 C/V, prefixes); in F1-08, D3 is
  Charles Petzold, "Code" (title only).
- D4 the datasheets / manuals of the kit and meter chosen by the owner (do not exist yet).
- R1 the lab runs above; S1 the authoring guide's safety statement.

The Source Researcher must confirm edition and section for D1–D3 for every tag. Claims that are pure
arithmetic (Q = I × t, Ohm's law sums, 63.2 % at one τ, ln 1.5 = 0.4055) are shown in worked
examples and printed by the runs.

## Unverified boxes (AH-18), per chapter

- F1-01: Part B (real battery and meter) not performed; meter settings from its manual (D4).
- F1-02: resistor value marking (colour bands / codes) deliberately not taught — no standard or
  datasheet opened; Part B not performed.
- F1-03: real resistor power ratings and temperature derating not quoted; Part B not performed.
- F1-04: breadboard connection pattern and Part B not performed / from kit documentation.
- F1-05: Part B (real RC timing) not performed; R, C tolerances, capacitor polarity and voltage rating,
  meter input resistance from D4.
- F1-06: all LED parameters are pretend (I_s = 1e-18 A, V_n = 0.05 V, V_F = 2 V); Part B not performed.
- F1-07: every meter number (input resistance 1/10/100 MΩ, burdens 0.1/1/10 Ω, 400 mA range, 500 mA
  fuse, "OL" display) is pretend; real values from the chosen meter's manual; Part B not performed.
- F1-08: no real logic levels or Schmitt thresholds (2.5 V, 2 V / 3 V are exercise values); Part B
  (real blinker) not performed; integrated timer-chip formula deliberately not given.

No real hardware number appears anywhere; every voltage/current/resistance/capacitance is labelled as
an exercise ("pretend") value or must be copied from a datasheet.

## Decisions for the owner

1. **Choose the HW101 kit and meter** (battery pack, breadboard, resistors, capacitors incl. any
   polarised ones, LEDs, a Schmitt-trigger inverter or a timer chip for the course project, a
   multimeter). Their datasheets/manual become D4 in every chapter; all "Part B" lab steps wait for this.
2. **Course project part**: the chapters describe the RC blinker with a Schmitt-trigger inverter
   (formula derived and simulated in F1-08). If the owner prefers an integrated timer chip, its formula
   must come from that chip's datasheet (not written from memory here).
3. **Analogy registrations (proposals to the Dean, guide 8.2)** — used in the chapters and marked
   "proposed":
   - Capacitor = "a tank with a stretchy rubber wall" (F1-05). Note guide 8.1 says capacitors have
     no simple pipe equivalent; the chapter quotes that and lists three breaks.
   - Diode = "a one-way flap valve in the drain" (F1-06), continuing F0-40's "one-way drain".
   - Multimeter = "the plumber's pressure gauge (voltage), flow meter (current) and pipe tester
     (resistance)" (F1-07).
   - Power = "how hard and how fast the spice mill is turned" (F1-03) and the "warm narrow pipe" idea.
   - Analog / digital = "the oven thermometer / the order bell" (F1-08).
4. **Approve publication with unverified boxes** (AH-19) — every chapter has at least one (the kit parts).
5. **Glossary overlaps with KID103**: terms Voltage, Current, Resistor, Ohm's law, LED, Diode,
   Anode and cathode, Forward voltage, Multimeter are also defined in F0-39/F0-40. The HW101 entries
   are consistent with them but more precise; the Integrator should merge them (guide 10.3).
6. **Exams** (Q, M, F, P) are not written here (Exam Writer's job); the practical exam "predict, build
   and measure a small circuit" maps naturally onto F1-07's lab plus F1-06's LED drive sheet.
7. Chapters run at the upper end of the L1 length range (≈3,400–4,200 words including jargon box,
   lists, answers and keys; prose alone is within 2,000–3,500). Average sentence length 14–17 words.

## Cross-links used

`#KID103`, `#F0-40`, `#HW101`, `#HW102`, `#F1-09` (first HW102 chapter), `#safety`, `#gl-…` (own glossary).
Later chapters mentioned by id in prose only (no link): F1-15, F1-18, F1-64, HW204, HW301, HW302.
