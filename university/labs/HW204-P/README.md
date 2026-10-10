# HW204 practical exam (P) — lab folder

Run: `university/labs/run_lab.sh university/labs/HW204-P` (runner exit code 0 in this build; the
start file's own run ends with exit code 1 on purpose, see below).
Paper: `university/chapters/HW204/EXAMS.html`, section "Practical (P)".
Marking points, reference outputs and the forensic key: `university/_keys/HW204.keys.html`.

Candidates receive ONLY these two starting files (owner ruling B7); nothing else in this folder
is handed out:

- `i2c_decode_start.cpp` — the decoder to complete (five functions marked TODO; the capture
  parser, the nine-bit grouping, the printing and a self-check are given).
- `i2c_decode_start.in` — the exam capture: the two I2C lines of a bus with one sensor at
  7-bit address 0x5A (exercise value), in the text format of F1-47 (rows of 64 samples, `-` high,
  `_` low, four samples per bit), recorded by the course's bus model.

  As handed out the start file builds, parses the capture, decodes nothing and ends with
  `self-check: 3 failures` (exit code 1): that is the expected starting state. Before the exam,
  candidates may also practise on `i2c_decode_practice.in`, which is the capture of F1-47
  Listing 1 whose decoding is printed in that chapter.

Reference solution and hidden evidence (Lab Engineer, not given to candidates):

- `i2c_decode.cpp` / `i2c_decode.in` — the completed decoder on the exam capture: the three
  transactions, `self-check: 0 failures`, exit code 0. Identical to the start file apart from
  the five TODO bodies and the header comment (`diff` shows it).
- `i2c_exam_model.h`, `make_capture.cpp` — the course's I2C bus model (F1-47's `i2c_sim.h`,
  extended to several targets) and the generator of the exam capture; `make_capture.out` ends
  with the ground truth. The model is the course's own and stands for a real I2C controller plus
  a sigrok/PulseView capture (ruling A2).
- `run.sh` — writes `poll_exam_O2.out` (disassembly evidence for midterm question M6, from
  `poll_exam.cc`), `i2c_decode_practice.out` (the reference decoder on the practice capture)
  and `capture_check.out` (proof that the handed-out capture equals the generator's rows).
- `uart_model.h` (copied from F1-42), `uart_fifo_forensic.cpp` — evidence for the final exam's
  forensic question "The FIFO that was not enough"; `uart_fifo_fixed.cpp` — the fix and the
  wrong fixes under the same workload, for the key.
- `exam_checks.cpp` — recomputes every number quoted in the HW204 answer keys that is not
  copied from a lab run.
- `project_ref.cpp` — the course project (interrupt-driven sensor reader) in the course's
  simulators: timer calculation, I2C read on the model, ring buffer, UART output with sequence
  numbers over a ten-minute simulated run, and a decoded trace. The hardware version (Pico 2 /
  RP2350 or NUCLEO-F446RE, LSM6DSOX breakout, FX2 logic analyser, ruling D1) is UNTESTED ON
  HARDWARE: no board was available in this build (rulings C3, D1).

Every `.out` and `.log` file is written by `university/labs/run_lab.sh` (or by `run.sh` in its
format). Nothing in this folder needs hardware. All addresses, register values, tick costs and
clock frequencies are exercise values (ruling A4), not a real part's.
