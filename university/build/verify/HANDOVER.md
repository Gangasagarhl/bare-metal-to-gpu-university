# Handover: finishing the university (state at 2026-10-10, second session)

The owner (Sagar) delegated every owner decision to the build lead and asked for: every unit
verified against real sources, exams for every course, and everything in one big
`university/UNIVERSITY.html` with contents tree, chapters, sections, labs and Q&A.
This file tells the next session exactly where the work stands and how to continue.

Repo: `Gangasagarhl/bare-metal-to-gpu-university`. PR #1 (branch
`claude/build-university-html-95jymw`) was merged into `main`. The second session worked on
branch `claude/serene-allen-f8b047` (branched from `main`). Everything below is committed and
pushed there. The second session was stopped by the owner because credits ran out, shortly
after its agents had started; no unit was finished in it.

## 1. Where things stand

Check any time with `python3 university/build/verify/vqueue.py`.

- **Verified (15 of 105 units):** KID101, KID102, KID103, MA101, MA102, HW101, HW102, HW201,
  HW202, HW203, HW204, HW205, HW301, HW302, RB101.
- **Exams written (7 of 87 courses):** KID101, KID102, KID103, MA101, MA102, HW101, RB101.
- **Interrupted (resume these first):**
  - Verification: HW303 (F1-73 partly edited, committed as WIP in the first session; the second
    session's verifier had read everything and was about to write its edit script when stopped,
    nothing of it was saved), SP101 (F2-01, F2-03, F2-07 partly edited, committed as WIP),
    SP102 and SP201 (reading only; nothing saved). The verify brief tells an agent to check and
    finish files from an interrupted run.
  - Exams: HW102, HW201, HW202, HW203, HW204, HW301. Their practical folders
    `university/labs/<C>-P/` are committed as WIP (starting files, reference solutions, run
    outputs; each has a README). The draft papers for HW201 and HW301 are parked in
    `university/build/verify/wip/<C>.EXAMS.draft.html` (moved out of the chapter folders because
    a paper without its keys file gives `build.py --check` broken `#keys-<C>` links). No keys
    file exists for any of the six. The next exam writer for each course reads its `-P` folder
    (and the draft paper where one exists), checks them, and finishes.
  - Also still to write exams: HW205, HW302 (verified, never started).
- **Not started:** every other unit (`vqueue.py verify 200` lists them), all their exams, the
  analogy additions, and the final rebuild.
- **Done in the second session:** the kit's breadboard component set is named in `KIT.md` and
  `KIT.fragment.html` (search-excerpt evidence, see section 4); the briefs carry this
  checkout's path and the web-access rules; `commit_course.sh` carries the new attribution.
- **Dropped by the owner (2026-10-10):** the Fable re-check of the first 9 units. They stay as
  verified. The one fix it was to carry (RB101 F9-07 and HW201 F1-22 Part B name no kit item)
  is now a small chapter edit for the final pass.

## 2. Files to read first

1. `university/OWNER_RULINGS.md`: the owner's rulings A1–A10, B1–B7, C1–C4, D1–D4.
2. `university/build/verify/VERIFY_BRIEF.md`: what one verifier does for one unit. Its
   "Web access" section has the build-lead ruling on search-excerpt evidence (section 4 below).
3. `university/build/verify/EXAM_BRIEF.md`: what one exam writer does for one course.
4. `university/build/KIT.md`: the reference kit (ruling D1).
5. `university/build/RESUME.md`: build conventions from the first pass.

## 3. How to run the loop

One background agent per unit (verify) or per course (exams).

- **Checkout path.** The briefs name `/home/user/bare-metal-to-gpu-university`. If the next
  container clones elsewhere, change the path in both briefs (one line each) before launching.
- **Toolchains.** A fresh container has none of the lab tools. Run
  `university/labs/setup_toolchains.sh` first (about 10 minutes; apt works through the
  package-manager allowance). Agents can start reading meanwhile; tell them to wait and retry if
  a tool is missing.
- **Model and effort (Sagar's instruction):** every verifier and exam writer runs with
  `model: "fable"`, `effort: "medium"`, `run_in_background: true`.
- **Verifier prompt:**
  "Your unit is <U> (all chapter files in university/chapters/<U>/) in the git repo at
  /home/user/bare-metal-to-gpu-university. Read university/build/verify/VERIFY_BRIEF.md and
  follow it exactly, including its 'Web access' section. If files from an interrupted earlier
  run already exist (dossiers, qa json, edits), check them and finish. Put any helper scripts in
  your own scratch folder /tmp/claude-0/work/verify-<U>/. Only re-run labs of your own unit. Do
  not commit or push. Never call any mcp__hearthbot__ tool."
- **Exam prompt** (only for a course whose units are all verified):
  "Your course is <C> (all chapters in university/chapters/<C>/) in the git repo at
  /home/user/bare-metal-to-gpu-university. Read university/build/verify/EXAM_BRIEF.md and follow
  it exactly. If university/labs/<C>-P/ or university/build/verify/wip/<C>.EXAMS.draft.html
  already exist from an interrupted run, check them and finish (move the draft back to
  university/chapters/<C>/EXAMS.html). Put any helper scripts in your own scratch folder
  /tmp/claude-0/work/exam-<C>/. Do not commit or push. Never call any mcp__hearthbot__ tool."
  For GPU courses add: "There is no GPU on this machine: any practical step that needs one is
  marked 'untested on hardware' as the chapters do."
- **Commit** each finished unit or course: `university/build/commit_course.sh <U> "…"`. It
  restores `UNIVERSITY.html`, adds the unit's chapters, dossiers, QA records, keys and `-P`
  practical folder, commits and pushes. Agents never commit themselves. Update the
  `Co-Authored-By` and `Claude-Session` lines in the script to the new session's values.
- **Next work:** `vqueue.py verify N` and `vqueue.py exam N` print the next pending items.
- **Concurrency:** at most 4 verifiers at once (web lookups), plus up to 6 exam writers.
- **Stopping early.** If a session must stop, stop the agents, restore lab re-run noise with
  `git checkout -- university/labs`, drop unreferenced new lab files, park any paper that has no
  keys file in `build/verify/wip/`, run `build.py --check` to 0 PROBLEM lines, commit the rest
  as WIP, and update this file. That is what the second session did.

## 4. Lessons (both sessions)

- **Network policy (second session).** The cloud environment's network policy allowed only
  package indexes and GitHub. `WebFetch` failed for every other host (ENOTFOUND or 403);
  `WebSearch` worked and returned quoted excerpts of the pages it found. The verify brief now
  rules: a document opened on GitHub counts as opened; a claim confirmed by a search excerpt
  that quotes the official page counts as confirmed, with "how accessed = search excerpt of
  <URL> (document not opened: network policy)" in the dossier; a claim confirmed only by
  secondary pages stays in an unverified box. **Before the next session, widen the
  environment's network access** (environment settings → Network access; allow the vendor and
  standards hosts, or a broader level). With full access, the brief's excerpt rule simply stops
  applying and verifiers open documents directly. The kit's breadboard rows and anything
  verified under the excerpt rule can then be upgraded to opened sources.
- **The web budget (first session).** With unrestricted access, WebFetch was limited to about
  400 calls per hour shared by every agent; 8 or more verifiers starved. Keep to 4 verifiers.
- **Background agents survive container restarts.** Check file times and `git status` before
  relaunching, so no unit gets two agents.
- **Scratch folders.** Every prompt names a private folder under `/tmp/claude-0/work/`.
- **UNIVERSITY.html.** Agents sometimes run `build.py` and leave the page modified. Restore it
  with `git checkout university/UNIVERSITY.html` before committing; only the final step
  commits it.
- **Practical folders** (`labs/<C>-P/`) hold reference solutions; `build.py` renders them inside
  the "Answer keys" appendix. A `chapters/<C>/EXAMS.html` without `_keys/<C>.keys.html` breaks
  the page's `#keys-<C>` links, so never leave one in place.
- **Lab re-runs** change `.log`/`.out` timing lines only; ruling A5 says keep the recorded files.

## 5. Follow-up work recorded so far

- **Kit:** the breadboard set is named (Adafruit Parts Pal 2975 plus single parts, SparkFun
  resistor kit, TI SN74HC14N), all from search excerpts; KIT.md's "Not verified" bullet says
  what is still open. The FPGA board for HW201 F1-22 is the Digilent Basys 3 (already in KIT.md).
  Still specification-only: x86 test PC, e-stop, flight battery brand, LiPo bag, safety glasses,
  bench restraint.
- **Kit names in chapters:** RB101 F9-07 and HW201 F1-22 Part B must name the kit items
  (micro:bit V2 + Kitronik :MOVE Motor; Basys 3). Small edit in the final pass.
- **Source sweep:** when access allows, reopen the documents the units list as "not opened".
  Each unit's NOTES.md "Verification pass" section and dossiers name them. Main gaps:
  - HW202: textbooks and ISA manuals.
  - HW203: Intel SDM Vol. 3, the memory-consistency Primer ch. 4/5/7/9, [atomics.order].
  - HW204: PC16550D, 8259A, USB 2.0, CAN 2.0, memory-barriers.txt, the GIC specification.
  - HW205: the NVMe Base spec, e1000x_regs.h, ACPI 6.5 ch. 11, VIRTIO 1.2, rdma-core man pages.
  - HW301: CUDA Programming Guide 5.1, the warp-vote/WMMA pages, nvcc, the LLVM kernel
    descriptor. Also, `mfma_amd` needs a CDNA GPU, not the kit's RDNA 4 card.
- **Analogy additions:** an agent compiles `chapters/ANALOGY_ADDITIONS.html` from the NOTES.md
  analogy proposals (ruling A3) after all units are verified. `build.py` already renders it
  under the analogy registry.

## 6. Finish line

1. All 105 units verified and all 87 courses with exams.
2. The kit-name chapter edits and the analogy additions are done.
3. `python3 university/build/build.py --check` reports 0 PROBLEM lines (it did at this handover).
4. Commit `university/UNIVERSITY.html`. The page was about 60 MB at this handover, and GitHub
   refuses files over 100 MB. If it grows past about 90 MB, split the dossiers appendix into a
   second page.
5. Copy the page to `/mnt/project-files/university/` if that folder exists.
6. Open a pull request from the working branch to `main` and tell Sagar.
