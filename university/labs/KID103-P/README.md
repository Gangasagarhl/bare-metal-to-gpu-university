# KID103-P: practical exam folder (KID103, "wire and explain a button-controlled LED")

Run: `university/labs/run_lab.sh university/labs/KID103-P` (runner exit code 0 in this build).
The exam paper is in `university/chapters/KID103/EXAMS.html` (section "Practical"); the
marking points are in `university/_keys/KID103.keys.html`.

Candidates receive only `button_led_start.cpp` (owner ruling B7); every other file here is for markers.

The course card's practical is a supervised kit build. No kit was available in this build, so the
practical has two parts: Part A is the emulated version (the course's own text simulator, run
here); Part B is the kit build, which is **untested on hardware** (owner ruling C3) and is marked
by the adult examiner against the kit's own instructions.

| file | role | expected result |
|---|---|---|
| `button_led_start.cpp` + `.in` | the STARTING FILE handed to the candidate (five TODO parts; builds and runs, but ignores the LED direction, forgets the forward voltage and lights the LED at every tick) | exit code 0; `30 mA`; `LIT` on every tick; `lit for 0 of 6 ticks` |
| `button_led_sim.cpp` + `.in` | reference solution; input: 6 V, LED forward, 200 ohms; button 0 1 1 0 1 0 | exit code 0; `20 mA`; LIT at ticks 1, 2, 4; `lit for 3 of 6 ticks` |
| `button_led_sim_reversed.cpp` + `.in` | identical copy; input: LED reversed | exit code 0; `LED reversed: it blocks …`; dark at every tick |
| `button_led_sim_hot.cpp` + `.in` | identical copy; input: 100 ohms (40 mA, above the pretend maximum) | exit code 0; `ABOVE the pretend maximum … not simulated`; dark at every tick |
| `forensic_nightlight.cpp` + `.in` | evidence for the forensic question of the final exam (two planted bugs: the two thresholds swapped, 34/26 instead of 26/34; and the current sum forgets the forward voltage) | exit code 0; `30 mA  ABOVE the pretend maximum`; `lamp changed 9 times` |
| `forensic_nightlight_fixed.cpp` + `.in` | answer key: both bugs fixed | exit code 0; `20 mA`; lamp on once, at minute 11; `lamp changed 1 times` |
| `nightlight_project.cpp` + `.in` | reference solution of the course project (night-light: two thresholds, LED loop sum, self-test, scripted evening) | exit code 0; `test_decide: 0 failures`; `10 mA`; `lamp changed 2 times; LED lit for 8 minutes` |

The specification the candidate receives (also in the exam paper):

1. Read the loop line `<battery volts> <LED forward 1/0> <resistor ohms>`; print
   `loop: battery 6 V, LED forward, resistor 200 ohms` (or `LED REVERSED`) and the pretend datasheet line.
2. Check the loop before any tick, in this order: reversed LED; battery not more than the forward
   voltage; no resistor; current `(battery - forward) x 1000 / ohms` in mA printed as
   `current when the button is pressed: 20 mA (pretend maximum 20 mA)`; above the maximum. Any
   failed check prints its line and keeps the LED dark at every tick (fail-safe default, F0-46).
3. Print `tick button LED`, then one line per reading: tick, `down`/`up`, `LIT`/`dark`. The LED is
   LIT only while the button is pressed and the loop passed its checks.
4. Print `LED was lit for <n> of <ticks> ticks`.

All LED, battery and resistor numbers are pretend exercise values (owner ruling A4): forward voltage
2 V, maximum 20 mA. Nothing in this folder needs hardware; everything was run on the build machine
with the course flags.
