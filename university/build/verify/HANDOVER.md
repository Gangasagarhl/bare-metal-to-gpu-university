# Handover: finishing the university (state at 2026-10-10)

The owner (Sagar) delegated every owner decision to the build lead and asked for: every unit
verified against real sources, exams for every course, and everything in one big
`university/UNIVERSITY.html` with contents tree, chapters, sections, labs and Q&A.
This file tells the next session exactly where the work stands and how to continue.

Repo: `Gangasagarhl/bare-metal-to-gpu-university`, branch `claude/build-university-html-95jymw`
(PR #1). Everything below is committed and pushed.

## 1. Where things stand

Check any time with `python3 university/build/verify/vqueue.py`.

- **Verified (15 of 105 units):** KID101, KID102, KID103, MA101, MA102, HW101, HW102, HW201,
  HW202, HW203, HW204, HW205, HW301, HW302, RB101.
- **Exams written (7 of 87 courses):** KID101, KID102, KID103, MA101, MA102, HW101, RB101.
- **Interrupted at handover (resume these first):**
  - Verification: HW303 (one chapter edited), SP101 (three chapters edited), SP102 and SP201
    (reading only, nothing written). Partial edits are committed as "WIP"; the verify brief tells
    an agent to check and finish files from an interrupted run.
  - Exams: HW102, HW201, HW202, HW203, HW204, HW301 (partial files were deleted; start fresh).
  - Kit: naming a real breadboard component set (see section 4); nothing was written yet.
- **Not started:** every other unit (`vqueue.py verify 200` lists them), all their exams, the
  analogy additions, and the final rebuild.
- **Dropped (owner, 2026-10-10):** the Fable re-check of the first 9 units. They stay as verified.

## 2. Files to read first

1. `university/OWNER_RULINGS.md`: the owner's rulings A1–A10, B1–B7, C1–C4, D1–D4.
   B6 (L0–L1 grading) and B7 (practical evidence) were added in this pass.
2. `university/build/verify/VERIFY_BRIEF.md`: what one verifier does for one unit.
3. `university/build/verify/EXAM_BRIEF.md`: what one exam writer does for one course.
4. `university/build/KIT.md`: the reference kit (ruling D1).
5. `university/build/RESUME.md`: build conventions from the first pass.

## 3. How to run the loop

One background agent per unit (verify) or per course (exams).

- **Model and effort (Sagar's instruction):** every verifier and exam writer runs with
  `model: "fable"`, `effort: "medium"`, `run_in_background: true`.
- **Verifier prompt:**
  "Your unit is <U> (all chapter files in university/chapters/<U>/) in the git repo at
  /home/claude/bare-metal-to-gpu-university. Read university/build/verify/VERIFY_BRIEF.md and
  follow it exactly. If files from an interrupted earlier run already exist (dossiers, qa json,
  edits), check them and finish. Put any helper scripts in your own scratch folder
  /tmp/claude-0/work/verify-<U>/. Only re-run labs of your own unit. The web tools share a
  lookup budget with other agents; if it runs out, wait a few minutes and retry rather than
  giving up early. Never call any mcp__hearthbot__ tool."
- **Exam prompt** (only for a course whose units are all verified):
  "Your course is <C> (chapters <first> to <last>) in the git repo at … Read
  university/build/verify/EXAM_BRIEF.md and follow it exactly. Put any helper scripts in your
  own scratch folder /tmp/claude-0/work/exam-<C>/. Never call any mcp__hearthbot__ tool."
  For GPU courses add: "There is no GPU on this machine: any practical step that needs one is
  marked 'untested on hardware' as the chapters do."
- **Commit** each finished unit or course: `university/build/commit_course.sh <U> "…"`. It
  restores `UNIVERSITY.html`, adds the unit's chapters, dossiers, QA records, keys and `-P`
  practical folder, commits and pushes. Agents never commit themselves.
- **Next work:** `vqueue.py verify N` and `vqueue.py exam N` print the next pending items.

## 4. Lessons from this pass (important)

- **The web budget is the bottleneck.** WebFetch is limited to about 400 calls per hour,
  shared by every agent. With 8 or more verifiers running, most fetches failed and units ended
  with many sources "not opened". Run **at most 4 verifiers at once**. Exam writers need no web
  access, so they can run alongside (up to about 6 more).
- **Background agents survive container restarts.** Check file times and `git status` before
  relaunching, so no unit gets two agents.
- **Scratch folders.** Agents overwrote each other's helper scripts in the shared scratchpad.
  Every prompt now names a private folder under `/tmp/claude-0/work/`.
- **UNIVERSITY.html.** Agents sometimes run `build.py` and leave the page modified. Restore it
  with `git checkout university/UNIVERSITY.html` before committing; only the final step
  commits it.
- **Practical folders** (`labs/<C>-P/`) hold reference solutions. `build.py` now renders them
  inside the "Answer keys" appendix, not after the exam paper.

## 5. Follow-up work recorded during this pass

- **Kit gaps:** `KIT.md` still lists the breadboard set (LEDs, resistor assortment, push
  buttons, switches, AA battery holders, light sensor, 74HC14, capacitors) as
  specification-only. KID103 (F0-40, F0-41, F0-43) and HW101 Part B need named parts. Also no
  FPGA board is named for HW201 F1-22. Fix `KIT.md` and `KIT.fragment.html` (ids start `kit-`).
- **Units verified before KIT.md existed:** RB101 F9-07 and HW201 F1-22 Part B name no kit.
  Fix this as a small chapter edit in the final pass (the re-check that was to carry it is dropped).
- **Fable re-check:** dropped by the owner on 2026-10-10. The first 9 units keep their
  verification from the earlier pass.
- **Source sweep:** when the web budget allows, reopen the documents the units list as
  "not opened (budget)". Each unit's NOTES.md "Verification pass" section and dossiers name
  them. Main gaps:
  - HW202: textbooks and ISA manuals.
  - HW203: Intel SDM Vol. 3, the memory-consistency Primer ch. 4/5/7/9, [atomics.order].
  - HW204: PC16550D, 8259A, USB 2.0, CAN 2.0, memory-barriers.txt, the GIC specification.
  - HW205: the NVMe Base spec, e1000x_regs.h, ACPI 6.5 ch. 11, VIRTIO 1.2, rdma-core man pages.
  - HW301: CUDA Programming Guide 5.1, the warp-vote/WMMA pages, nvcc, the LLVM kernel
    descriptor. Also, `mfma_amd` needs a CDNA GPU, not the kit's RDNA 4 card.
- **Analogy additions:** an agent compiles `chapters/ANALOGY_ADDITIONS.html` from the NOTES.md
  analogy proposals (ruling A3). `build.py` already renders it under the analogy registry.

## 6. Finish line

1. All 105 units verified and all 87 courses with exams.
2. The kit fixes and analogy additions are done (the Fable re-check was dropped by the owner).
3. `python3 university/build/build.py --check` reports 0 PROBLEM lines.
4. Commit `university/UNIVERSITY.html`. The page was about 57 MB before exams and dossiers, and
   GitHub refuses files over 100 MB. If it grows past about 90 MB, split the dossiers appendix
   into a second page.
5. Copy the page to `/mnt/project-files/university/`.
6. Update the PR #1 description and tell Sagar in the build thread.
