# Exam pass brief (one course)

You are the Exam Writer and Lab Engineer for ONE course of the university in the git repo at
/home/user/bare-metal-to-gpu-university. Work only in that checkout. Do not commit or push.
Never call any mcp__hearthbot__ tool. Touch only `university/chapters/<COURSE>/EXAMS.html`,
`university/_keys/<COURSE>.keys.html` and `university/labs/<COURSE>-P/` (the practical's
reference solution). Another agent may be verifying the course's chapters at the same time: do
NOT edit chapter files, glossary.json or NOTES.md.

## Read first
`university/OWNER_RULINGS.md` (ruling B4), `university/build/KIT.md` if present,
`university/chapters/FRAGMENT_FORMAT.md`, guide sections 11.1–11.5 in
`UNIVERSITY_AUTHORING_GUIDE.html` (extract text with a small python script), your course card in
guide section 5 (its "Exams" and "Project" fields), and every chapter of the course (focus on
learning goals, Check yourself, labs, forensic labs, mini-projects and NOTES.md).

## Write
1. `university/chapters/<COURSE>/EXAMS.html` — an HTML fragment starting with
   `<!--meta\nid: <COURSE>-exams\ncourse: <COURSE>\n-->`; every id prefixed `<COURSE>-exams-`;
   no URLs, no `<script>`. Sections (h2), as the course card and guide 11.4 require:
   - Quizzes: one line per chapter pointing to that chapter's Check yourself as quiz Q
     (link `#<CHAPTER>-check`), weight per guide 11.4.
   - Midterm (only for courses of 4+ credits; otherwise say the weight moves to the final):
     a blueprint table (topics × Bloom levels, guide 11.4 rules) and the questions.
   - Final: blueprint table and questions; cumulative; at least one forensic question with its
     evidence shown (logs/outputs from the course labs, copied exactly).
   - Practical (P): the task, time limit, what documentation is allowed, the starting files
     (in `university/labs/<COURSE>-P/`), and how it is marked. L0 courses: quizzes, practical and
     project only, with encouraging feedback.
   - Course project: brief per guide 11.5 (goal, requirements, constraints, deliverables,
     milestones, rubric — the default rubric unless the card says otherwise — and the curriculum
     acceptance tests it reuses).
   Questions must be answerable from the course's chapters; levels must match the course level.
2. `university/_keys/<COURSE>.keys.html` — fragment starting with
   `<!--meta\nid: <COURSE>-keys\ncourse: <COURSE>\n-->`, ids prefixed `<COURSE>-keys-`: model
   answers and marking points for the midterm, final and practical, the forensic answer, and
   project reference notes. Every number in a key must come from a real run.
3. `university/labs/<COURSE>-P/` — the practical's starting files and the reference solution,
   plus `run.sh` if needed; run it with `university/labs/run_lab.sh university/labs/<COURSE>-P`
   until it passes. Hardware-only practicals: provide the emulated/simulated version that runs
   here and mark the hardware version untested on hardware.
Validate both fragments (html.parser balance, prefixed ids, no URLs, no script).

## Owner rulings B6 and B7
L0–L1 courses: midterm and final are written but are ungraded practice papers; weights are
quizzes 20 %, practical 50 %, project 30 % (ruling B6). Other courses use the 11.4 defaults.
The `-P` README says candidates get only the starting file(s) (ruling B7).

## UNIVERSITY.html
If you run build.py, restore university/UNIVERSITY.html with `git checkout` afterwards. The practical folder `labs/<C>-P/` is rendered inside the answer-keys appendix, not after the exams.

## Report
Short summary: files written, number of questions per exam, practical run result, anything the
owner should know.
