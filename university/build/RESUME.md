# How to resume the build (for a fresh session)

The university is written one **course** at a time. Each finished course is committed and
pushed on its own, so the branch `claude/build-university-html-95jymw` always holds only
complete courses. `PROGRESS.md` (regenerated on every course commit) shows what is done.

## Steps

1. Check out the branch and see where it stands:
   ```
   git fetch origin claude/build-university-html-95jymw
   git checkout claude/build-university-html-95jymw
   university/build/resume.sh            # prints done/pending, next courses, cleans stale partial work
   ```
2. If the container is new, reinstall the toolchains: `university/labs/setup_toolchains.sh`.
3. For each pending course (about 10 in parallel), start one author agent with this prompt:
   > Your complete task instructions are in the file
   > university/build/prompts/<COURSE>.txt (in the repo checkout). Read that file first and
   > follow it exactly. If files from an interrupted earlier run already exist in your
   > course's folders, check them and finish or rewrite them. Never call any
   > mcp__hearthbot__ tool.
4. When an agent reports back, run
   `university/build/commit_course.sh <COURSE> "<n> chapters on <topic>"`
   (it discards nothing; it adds the course's chapter and lab folders, refreshes
   PROGRESS.md, commits and pushes). If an agent rebuilt `university/UNIVERSITY.html`,
   restore it first with `git checkout -- university/UNIVERSITY.html`.
5. Bridge chapters BR-01..10 and mega projects MP1..8 are in the same queue and use the same
   steps (one folder each, e.g. `university/chapters/BR-03/`, prompts in `prompts/BR-03.txt`;
   the builder renders them under "Bridge chapters" and "Mega projects").
6. After every queue entry is done: the glossary-link fixes, `python3 university/build/build.py --check`,
   commit `university/UNIVERSITY.html`, and mark the pull request ready.

Owner decisions are collected in each course's `university/chapters/<COURSE>/NOTES.md`.

## Verification pass (owner delegated, 2026-10-10)

Rulings: `university/OWNER_RULINGS.md`. Briefs: `build/verify/VERIFY_BRIEF.md` (one unit per
agent: dossiers in `university/_dossiers/`, QA in `university/qa/`) and `build/verify/EXAM_BRIEF.md`
(one course per agent, after the course is verified: `chapters/<C>/EXAMS.html`,
`_keys/<C>.keys.html`, `labs/<C>-P/`). Queue status: `python3 university/build/verify/vqueue.py`;
next units: `vqueue.py verify N` / `vqueue.py exam N`. Agent prompt:
"Your unit is <U> (chapters …) … Read university/build/verify/VERIFY_BRIEF.md (or EXAM_BRIEF.md
with <COURSE>) and follow it exactly …". Commit each finished unit with `commit_course.sh <U> "…"`
(it now also adds dossiers, QA records, keys and practical labs). Reference kit:
`build/KIT.md` + `build/KIT.fragment.html`. Last step: an agent compiles
`chapters/ANALOGY_ADDITIONS.html` from the NOTES.md analogy proposals; then `build.py --check`,
commit UNIVERSITY.html, copy to /mnt/project-files/university/.
