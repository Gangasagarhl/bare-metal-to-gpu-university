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

## Owner rulings applied

Verification pass of 2026-10-10 (rulings in `university/OWNER_RULINGS.md`).

1. **Choose the HW101 kit and meter** → ruling **D1**. `build/KIT.md` names the Fluke 17B+ multimeter and the
   Digilent Analog Discovery 3 (RC timing); the HW101 breadboard component set (battery, resistors, capacitors,
   LEDs, Schmitt-trigger inverter) is still a specification, not a named product. Every chapter's D4 entry now
   says so. The Fluke 17B+ manual was not opened, so all meter numbers stay labelled pretend values (F1-07).
2. **Course project part** (Schmitt-trigger inverter or timer chip) → no ruling covers it; **decided by verifier:**
   keep the Schmitt-trigger inverter. The Nexperia 74HC14 data sheet (D15 of F1-08, section 13) shows the same
   R–C relaxation circuit; its own period formula (K-factor graph) was not legible and is noted in F1-08's Part B
   box. No timer-chip formula is given.
3. **Analogy registrations** (rubber-wall tank, flap valve, plumber's gauges, spice mill / warm pipe, oven
   thermometer / order bell) → ruling **A3**: approved. "Analogy (proposed)" labels changed to "Analogy" in
   F1-05, F1-06, F1-07, F1-08; meta lines and `glossary.json` say "approved mapping, owner ruling A3".
4. **Publication with unverified boxes** → rulings **C1/C3/C4**: allowed. Every Part B box now says
   "Untested on hardware" and what is needed to test it. Status: "internally checked · hardware steps untested".
5. **Glossary overlaps with KID103** (Voltage, Current, Resistor, Ohm's law, LED, Diode, Anode and cathode,
   Forward voltage, Multimeter) → ruling **A7**: KID103's wording (earlier in build order) is canonical when the
   meaning is the same; the HW101 entries stay as the more precise definitions and are merged by the Integrator.
   No change made in this unit's files beyond the source fields.
6. **Exams** → ruling **B4**: written by the Exam Writer pass (EXAM_BRIEF.md), not in this verification pass.
7. **Length above the L1 target** → ruling **A1**: accepted as written; nothing trimmed for length.
- Ruling **A5** applied to the lab re-runs (see below). Rulings A8 (keys), A10 (licence placeholder) and D4
  (e-stop) do not apply: HW101's labs contain no keys, no `LicenseRef-Uni-Lab` marker and no robot image.

## Verification pass

Fact-Checker / Source Researcher agent, 2026-10-10. Dossiers: `university/_dossiers/F1-01…F1-08.dossier.html`;
QA records: `university/qa/F1-01…F1-08.json` (all "factcheck": "done").

**Opened (web, 2026-10-10):** BIPM SI Brochure 9th ed. (V4.01, June 2026) §2.3.1, §2.3.4 Table 4, Table 7;
OpenStax University Physics Vol. 2 §7.2, 8.1, 9.1–9.5, 10.1–10.5 (summaries of ch. 9 and 10); Vol. 1 §14.7;
Vol. 3 §9.6–9.7; McLaughlin and Howe, "Applied Electrical Engineering Fundamentals" §3.3–3.5; Vishay resistor
datasheet 28766; NIC Components aluminium electrolytic guideline; TI AN-1656 (SNVA253A), SCEA046B, SZZA036C,
SDAA011A, SCAA035B; Nexperia 74HC_HCT14 Rev. 10; ADI MT-001, MT-002; Fluke "Can you live with the burden?";
UC Berkeley EECS 100 "Multimeters"; Nobel Prize 2014 popular information "Blue LEDs".
**Not opened:** "Make: Electronics" and "The Art of Electronics" (not freely readable; kept as further reading,
no claim relies on them); Petzold "Code" (same); Fluke multimeter manuals (failed to load); the OSRAM LED
application note (robots disallowed); the web-fetch budget ran out near the end of the pass.

**Claims:** about 247 tagged claims checked across the eight chapters; every D1/D2/D3(Code) tag replaced by an
opened source with its section (tags now read e.g. "D5 §9.1"). 26 corrections, the main ones:
- F1-02: pipe flow *is* proportional to pressure while laminar; turbulence raises its resistance (analogy break).
- F1-03: "a small LED loop uses a few hundredths of a watt" contradicted Figure 1 (0.12 W); Musa's 30× power
  now explained with P = V² ÷ R.
- F1-04: code-table row said Req prints without decimals (it prints 3000.000).
- F1-05: capacitance/propagation-delay claim not supported → power only (TI SCAA035B), delay deferred to F1-15.
- F1-06: forward voltage not set by material in the opened source (colour only); diode-use examples removed.
- F1-07: Lab Part A output has thirteen lines, not twelve; DMM internals trimmed to what D5 §10.4 and D13 say.
- F1-08: Schmitt "internal feedback" and ADC "comparison with references" not found → generalised.
Unverified items kept in boxes: kit/meter values (D4) in every chapter; resistor colour code (IEC 60062,
paywalled); diode symbol convention, LED brightness/reverse-voltage statements; meter accuracy format, range
habit and "OL" display; D15's period formula; timer-chip formula.

**Labs:** all 19 listings of F1-01…F1-08 re-run with `run_lab.sh` on 2026-10-10: every one passes (exit 0) with
identical output; only the log dates changed, so the recorded `.out`/`.log` files were restored (ruling A5).
No lab code, input or output changed. All Part B (kit) steps remain untested on hardware.

**Build:** `python3 university/build/build.py` reports no PROBLEM line for HW101, F1-01…F1-08 or their dossiers;
`UNIVERSITY.html` restored afterwards.
