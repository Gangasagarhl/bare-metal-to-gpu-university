# HW202 The CPU: instruction sets, datapaths and pipelines — author notes

Author / Lab Engineer run, build of 2026-10-09 (no internet; brief `/tmp/claude-0/prompts/HW202.txt`).
Level L2, 4 credits, prerequisites HW201 and SP101. Faculty F1, analogy world: the restaurant building
and its kitchen. Card "Maps to: Curriculum 1.3, 1.4": those curriculum sections are reading lists with
no milestones or acceptance tests, so none were copied. The labs follow the card's lab wording
("Complete the simulator CPU; run a program on it; compare compiler output at several optimisation
levels"); the course forensic lab "The slow loop" (load-use hazard) is F1-27's forensic lab; the course
project ("a working simulated CPU running a program you wrote in its machine language") is built up in
the mini-projects of F1-23 to F1-25 on the U16 machine.

## Files

- Chapters: `F1-23.html` … `F1-31.html` (all sections of the fragment format, Transition box, Answers
  to Check yourself with the forensic answer key inside Answers).
- Glossary: `glossary.json`, 67 four-part entries generated from the chapters' Jargon boxes (plus
  "Microarchitecture", written by hand); every `#gl-…` link in the nine chapters resolves to one of them.
- Labs: `university/labs/F1-23` … `F1-31` (sources, `.in`, `.out`, `.log`, and a `run.sh` where a lab
  needs more than the default compile-and-run).

## Listings run

All labs were rerun at the end of the build with `university/labs/run_lab.sh university/labs/<ID>` from the
repository root; every one returned exit code 0. Default toolchain:
`g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0`, `-std=c++20 -Wall -Wextra -Wpedantic -Werror -g
-fsanitize=address,undefined`, Linux x86_64 build container. Other tools, each recorded in its log:
Icarus Verilog and Yosys (F1-25), GNU objdump 2.42, `aarch64-linux-gnu-g++` and `riscv64-linux-gnu-g++`
13.3.0, `Ubuntu clang version 18.1.3`.

| Chapter | Runs (log names) | Result |
|---|---|---|
| F1-23 | `asm_listing` (U16 assembler + machine, sum program, output 29), `forensic` (the 6×7 hex program with one wrong word; stops at the 10000-step limit, by design), `real_x86`, `real_arm64`, `real_riscv64` (add3.cc compiled and disassembled) | pass, exit 0; the three real ISAs are **compiled and disassembled only, not executed** |
| F1-24 | `datapath` (per-instruction datapath trace), `cpu` (multiply 6×7, output 42), `forensic` (count_below with r0 used as a counter) | pass, exit 0 |
| F1-25 | `control_table` (C++), `control_tb` (Verilog control unit under Icarus), `control_diff` (no differences), `control_bug_tb` + `control_bug_diff` (row 8, `sw`, differs: the forensic evidence), `control_synth` (Yosys statistics) | pass, exit 0 |
| F1-26 | `pipeline` (5 addi + halt, 10 cycles), `sum_ideal` (34 cycles), `stage_time` (850 vs 270 du per instruction; speed-up table) | pass, exit 0 |
| F1-27 | `hazards` (sum: 62 cycles without forwarding, 47 with), `reordered` (47 / 42), `forensic`, `forensic_fixed` ("The slow loop": 123 / 81 and 115 / 73), `answer_*` (the same programs on the CPU: identical outputs) | pass, exit 0 |
| F1-28 | `predictor` (30 / 10 / 20 / 12 mispredictions; 42 vs 34 cycles), `forensic` (sorted 2 vs unsorted 34 mispredictions) | pass, exit 0 |
| F1-29 | `ilp` (issue model), `accum` (real `-O2` timing, one vs four sums), `accum_asm_one`, `accum_asm_four` (objdump) | pass, exit 0; timing varies run to run (ratio about 1.4–1.6 in this session) |
| F1-30 | `threads`, `interleave`, `race_plain` (three runs, lost updates), `race_tsan` (ThreadSanitizer report) | pass; `race_tsan.log` records exit code 66, which is ThreadSanitizer's own code after reporting the race (the program returns 0) — this is the intended evidence |
| F1-31 | `x86_O0/O1/O2/O3`, `x86_O2_att`, `clang_x86_O2`, `arm64_O0/O2`, `riscv64_O0/O2` (funcs.cc), `vanish_O0`, `vanish_O2`, `vanish_main_O2` | pass, exit 0; ARM64 and RISC-V **compiled and disassembled only, not executed** |

No expected-fail listings. No CUDA/HIP. "Untested on hardware" applies to: all ARM64 and RISC-V code
(F1-23, F1-31: no board, no `qemu-user` in the container), and every claim about a real processor's
predictor, pipeline or core width (F1-27 to F1-29: `perf` printed "perf not found for kernel
6.18.44-fc", so no hardware counters could be read). The only real-hardware timings are F1-29 `accum`
and F1-31 `vanish` (and F1-30 `race_plain` counts); each is labelled as one session on a shared cloud
container (AH-23) and the prose is written so that it stays true when the numbers change on rerun.

Rerun-sensitive outputs (the builder inserts whatever the last run produced): `F1-29/accum.out`,
`F1-30/race_plain.out`, `F1-30/race_tsan.out` (addresses, thread ids), `F1-31/vanish_O0.out`. The
chapters quote none of their exact numbers except "0.000 ms" (F1-31, `-O2`, deterministic because the
loop is removed) and "hits = 200000" in the ThreadSanitizer run (seen in every run during the build;
the text says so).

## Claims that could not be verified (sources title only, gate G1 open)

No source document could be opened (no internet; Compiler Explorer unreachable, so all assembly was
produced locally). Every technical claim is tagged to a title-only source or to a run:

- Patterson & Hennessy, "Computer Organization and Design" — main source for ISA, datapath, control,
  pipelining, hazards, forwarding, branch prediction, multicore, Amdahl's law (F1-23 … F1-30).
- Hennessy & Patterson, "Computer Architecture: A Quantitative Approach" — deeper pipelines, dynamic
  prediction, BTB, renaming, ROB, ILP limits, coherence and consistency (F1-26 … F1-30).
- Harris & Harris, "Digital Design and Computer Architecture" — architecture vs microarchitecture,
  datapath and control in hardware, HDL (F1-23 … F1-26).
- Nisan & Schocken, "The Elements of Computing Systems" — assembler (F1-23).
- Arpaci-Dusseau, "Operating Systems: Three Easy Pieces"; the C++ standard (ISO/IEC 14882) — threads,
  data races, atomics, as-if rule, signed overflow (F1-30, F1-31).
- Intel SDM; Arm ARM; RISC-V Unprivileged ISA manual; System V AMD64 psABI, AAPCS64, RISC-V psABI;
  GCC manual "Optimize Options"; GNU Binutils `objdump` documentation (F1-23, F1-29, F1-31).

The Source Researcher must confirm edition and section for each tag. Statements about real ISAs in
F1-23 and F1-31 are limited to what the build's own disassembly shows (register names, instruction
lengths, `cmovge`/`csel`/`bge`, `paddd`, `endbr64`).

## Unverified boxes, per chapter

- F1-23: register counts and widths of x86-64, ARM64, RISC-V not quoted; names come only from the
  disassembly. Warning box: cross-compiled objects were not executed.
- F1-24: real-processor remarks (split I/D caches, multi-cycle designs) from unopened sources; no real
  gate delays; Check-yourself delays made up.
- F1-25: which commercial CPUs use microcode, and for what, is not stated.
- F1-26: the stage delays are invented delay units; real pipeline depths and the "too deep" claim are
  from unopened sources; the pipeline is a rule-based timing model.
- F1-27: all cycle counts are model results; no real CPU's load-use penalty or forwarding is stated;
  the remark on exposed load-delay ISAs is from an unopened source.
- F1-28: real predictor organisation/accuracy not stated; `perf` unavailable.
- F1-29: real width, window, ROB size, latencies not stated (CPU model string only); model latencies
  invented; `perf` unavailable.
- F1-30: no thread speed-up measured; coherence protocol / memory model of real CPUs not stated; the
  history of the move to multicore not checked against the sources.
- F1-31: ABI rules observed for two `int` arguments only; `endbr64` and "base RISC-V has no
  conditional move" from general knowledge; ARM64/RISC-V not executed.

## Analogy mappings proposed for the F1 registry (restaurant kitchen)

New mappings used in these chapters; please add them to the analogy registry or replace them:

| Concept | Mapping | First used |
|---|---|---|
| ISA | the chef's exact ticket language | F1-23 |
| Microarchitecture | how one kitchen is laid out and staffed for the same tickets | F1-23 |
| Program counter | the clip on the ticket rail | F1-23 |
| Datapath | the ticket's road through the stations | F1-24 |
| Control unit | the expediter at the pass (Amina) | F1-25 |
| Pipeline | a row of five stations, all moving on at each bell | F1-26 |
| Forwarding | handing the pot across the counter instead of via the shelf | F1-27 |
| Load-use hazard | the pantry runner coming back one bell late | F1-27 |
| Branch prediction | starting the regular's usual dish before the order arrives | F1-28 |
| Superscalar / out-of-order | more cooks per station; cooking whatever is ready; plates leave the pass in ticket order (ROB) | F1-29 |
| Register renaming | a fresh labelled pot per ticket | F1-29 |
| Multicore | several head chefs sharing one pantry | F1-30 |
| Data race | two chefs updating the pantry's lemon board at once | F1-30 |
| Compiler / -O levels | branch kitchens rewriting head office's recipe into their own tickets | F1-31 |
| Dead code elimination | the branch that stopped making the stock nobody tasted | F1-31 |

Recurring character: Amina, the expediter who runs the kitchen (F1-25, F1-29, F1-31). Other story and
forensic characters are named per chapter (for example Ms Okafor in F1-26, Mr Haddad and Priya in F1-28,
Jonas in F1-29, Tomás in F1-26 and F1-30, Ana in F1-31); none is a real person. Every chapter has a "Where the analogy breaks" section.

## Decisions for the owner

1. **The U16 teaching ISA** (F1-23 onward): 16-bit words, eight registers with r0 = 0, Harvard
   memories of 256 data words, 12 instructions (HALT, ADD, SUB, AND, OR, SLT, ADDI, LW, SW, BEQ, BNE,
   OUT), three formats; branch target = pc + 1 + imm. It is invented for this course so the whole CPU
   fits in one header (`labs/F1-23/u16.h`) that F1-24 … F1-28 reuse. Alternative: a RISC-V subset
   (more transferable, but larger and needs byte addressing). Please confirm or choose.
2. **The pipeline timing model** (`labs/F1-26/pipe.h`): classic five stages, register file written in
   the first half-cycle and read in the second, forwarding rules (ALU → EX+1, load → MEM+1), predict
   not taken, branches resolved in EX (2-cycle penalty). It computes timing from rules rather than
   simulating pipeline registers. The identity cycles = instructions + 4 + stalls + branch cycles holds
   for every run. A gate-level HDL pipeline is suggested as a mini-project, not provided.
3. **Verilog in F1-25**: the control unit is written in Verilog and checked against the C++ table with
   Icarus Verilog, and synthesised with Yosys for a cell count. This needs `iverilog` and `yosys` on
   the student machine. Keep, or make it optional?
4. **Invented delay units** (F1-24, F1-26): stage delays are made-up "du", stated as such.
5. **Real timings** (F1-29, F1-31, F1-30 counts) vary between runs; prose is qualitative. Owner may
   prefer to drop real timings from course pages entirely.
6. **perf unavailable** in the build container; F1-28/F1-29 say so. If a machine with working `perf`
   is available, a measured branch-miss lab would strengthen F1-28.
7. **Cross-compiled code not executed** (ARM64, RISC-V). Installing `qemu-user` in the build image
   would let the labs run it.
8. **Compiler Explorer** named on the course card was unreachable; F1-31 uses local compilers and says
   so in the lab text.
9. **Course-card title**: F1-30 uses the card's title "Multicore processors".
