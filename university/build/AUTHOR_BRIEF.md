# Brief given to the chapter-author agents (batch 1: the Foundation year)

This is the exact brief each Author/Lab-Engineer agent received for batch 1, kept
here so the process is reproducible (guide 12.1, 15.2).

## Your inputs (read before writing anything)

1. `UNIVERSITY_AUTHORING_GUIDE.html` (repo root). Read at least: "Honesty first",
   sections 2 (levels, 2.1 table), 3 (anti-hallucination protocol), 6.1 (Transition box),
   7 (chapter template), 8 (analogy system — your faculty's world and the rules in 8.2),
   9 (diagram rules), 10 (jargon rules, reading level 10.4), 11.1–11.3 (labs, forensic labs,
   quizzes), 13 (style guide, phrases to avoid) and 14 (the exemplar chapter F6-01 —
   match its shape and tone).
2. Your course card in guide section 5 (copied into your task).
3. `university/chapters/FRAGMENT_FORMAT.md` — the exact file format. Follow it literally.
4. `university/labs/run_lab.sh` — the Lab Engineer runner.

## The honesty rules for this batch (important)

This build runs in a sandbox **without internet access**: no official document,
textbook or web page can be opened. Therefore:

- **Never write a URL.** (AH-30)
- **Your memory is not a source** (AH-3). Prefer claims that are true by definition or
  by arithmetic and are *shown* in the chapter (a binary conversion worked out step by
  step), or claims *demonstrated by a program you actually ran* in this build (for
  example the number C++ prints for the character `'A'`): cite those as the lab run
  ("R1 — lab run of Listing 2 in this build", with the toolchain line from the `.log`).
- For any other factual claim (how a part of a computer works, how networks deliver
  messages, what a sensor does), cite a named book from the guide's source registry
  (section 4.2/4.3, e.g. Charles Petzold, "Code: The Hidden Language of Computer Hardware
  and Software"; Noam Nisan and Shimon Schocken, "The Elements of Computing Systems";
  Bjarne Stroustrup, "A Tour of C++"; Charles Platt, "Make: Electronics"), and in the
  Sources list mark it exactly: *"title only — not opened during this build; the Source
  Researcher must confirm the edition and section (dossier gate G1 open)"*. Keep such
  claims conceptual and conservative.
- **Never state a number about real hardware** (speeds, sizes, counts, voltages of a real
  part, prices, dates, version numbers) unless it was printed by your own program run.
  If a chapter genuinely needs one, use an **unverified box** that names the document to
  check (AH-18), or ask the learner to measure it ("write your own number here").
- No invented quotes, no "experts say", no "studies show", no "typically" (13.8).
- Kits and simulators: do **not** name a commercial kit, board, simulator or app. Say
  "the kit your teacher or parent chose (its name and datasheets go in the dossier)" and
  put the choice in an unverified/decision box. Where a simulator is needed, use **the
  university's own tiny C++ text simulators** that you write and run in `labs/` (a
  grid-world robot, a line-following robot, a circuit/LED truth model, etc.). That is
  allowed: it is our code and we ran it.
- Safety boxes wherever guide "Honesty first" requires them (KID103 and RB101 physical
  activities: adult supervision, battery kits designed for children, simulator first,
  no mains, no LiPo, no propellers in F0).

## Code (AH-24 to AH-29)

- Every listing is a complete file in `university/labs/<CHAPTER-ID>/`, built and run with
  `university/labs/run_lab.sh university/labs/<CHAPTER-ID>` (g++, `-std=c++20 -Wall
  -Wextra -Wpedantic -Werror -fsanitize=address,undefined`). It must pass cleanly unless it
  is a deliberate `.expect-fail` listing. Re-run after every change. Never hand-write output:
  reference it with `data-run` so the builder copies the real `.out`.
- Kid-level code: tiny, every line explained in a table. Modern C++ (std::vector,
  std::string, no raw new/delete, no `using namespace std;`). Four-space indentation,
  function braces on their own line (style of guide 13.4 / the exemplar).
- Programs that need input get a `<name>.in` file. A forensic "loop that never ends"
  can use a `<name>.timeout` file (exit code 124 recorded honestly).
- Chapters with no natural code at L0 may still include one tiny listing that
  demonstrates the idea (e.g., printing a recipe's steps, counting in binary), or say
  in one sentence that code arrives in KID102 and point there.

## Writing

- Level, length and reading level per guide 2.1 and 10.4: L0 about 12 words per
  sentence, paragraphs of at most four sentences, second person, active voice; L0
  chapters 1,200–2,500 words of prose, L1 2,000–3,500 (excluding code). Warm,
  precise, no hype, culturally neutral, varied names (guide 8.2, 13.1).
- All 21 sections in order, plus "Answers to Check yourself" at the end. Story hook
  from the faculty's analogy world (F0: everyday life at home; F9: riding a bicycle,
  a body with senses and muscles). "Where the analogy breaks" has at least two
  concrete points. Every new term in the Jargon box in four parts. Transition box
  answers the four questions. Check yourself: 5–12 questions incl. one "explain in your
  own words" and one "apply to a new case"; distractors explained in the answers.
- Lab, forensic lab (scenario, evidence pack shown inline as a `<pre>` or table that you
  generated from a real run where it is program output, questions, method hints; the
  answer key goes inline under a clearly separated "Forensic lab answer key" `<h3>`
  inside the Answers section), mini-project with a short rubric.
- At least one inline SVG figure per chapter, following the diagram rules
  (required types of guide 9.2 where they apply: bits → row of switches with place
  values in F0-02/F0-20; inside a computer → kitchen map in F0-05; control loop for
  F9-05 where appropriate).
- Cross-link: prerequisites, the previous/next chapter (`#F0-02`), the course (`#KID101`).

## Deliverables

- `university/chapters/<COURSE>/<ID>.html` for every chapter of your course.
- `university/labs/<ID>/...` sources, `.in`, `.out`, `.log` (run them!).
- `university/chapters/<COURSE>/glossary.json` (four-part entries).
- `university/chapters/<COURSE>/NOTES.md`: per chapter, the claims you could not
  verify, unverified boxes, decisions left to the owner, and listings run.
- Do not edit files outside your course's folders. Do not commit or push.
- Do not call any `mcp__hearthbot__` tool.
