# Owner rulings

The owner (Sagar) delegated every open owner decision to the build lead on 2026-10-10. These
rulings answer the questions collected in [`OWNER_DECISIONS.md`](OWNER_DECISIONS.md). Where a
ruling below covers a unit's question, the unit's verifier applies it and records the outcome
under "Owner rulings applied" in that unit's `NOTES.md`. A question no ruling covers is decided by
the unit's verifier in the spirit of these rulings and recorded the same way.

## A. Content and pedagogy

- **A1 Length.** Chapters longer than the level's word target are accepted as written. Do not
  trim content to hit a word count; trim only text that is wrong, repeated or off-topic.
- **A2 Teaching stand-ins.** The university's own teaching implementations (uRTOS, the course
  fuzzer, U-Shot, U-RC, U-link, ULink, the toy hipify, teaching crypto, the quadrotor and drone
  simulators, the virtual cluster, the stand-in kit models, invented file formats) are approved,
  provided every page that uses one says plainly that it is the course's own and names the real
  tool it stands for. Real-tool names, parameters, commands and byte layouts must be verified
  against the real tool's documentation in the verification pass (see C1), or stay in an
  "unverified" box.
- **A3 Analogies.** All analogy mappings proposed in the units' `NOTES.md` are approved and are
  registered in the "Analogy registry additions" section of `UNIVERSITY.html`. Each course keeps
  its own cast of characters; names are not unified across courses.
- **A4 Exercise values.** Invented exercise values (gains, thresholds, budgets, timing limits,
  rubric level descriptors, plan lengths, percentiles) are approved as exercise values. They must
  be labelled "exercise value" where they could be mistaken for a real product's value.
- **A5 Recorded runs.** Chapters that quote timing numbers keep the recorded runs. A re-run whose
  only differences are run-to-run timing noise does not replace the recorded `.out`/`.log` files.
- **A6 Forensic giveaways.** A lab output that hints at a forensic answer may stay; it is honest
  evidence.
- **A7 Glossary duplicates.** When two courses define the same term with the same meaning, the
  first course's wording (build order) is the canonical one; the later course links to it.
  Terms with different meanings keep qualified names, e.g. "Hazard (functional safety)".
- **A8 Committed test keys.** Private keys committed for labs are approved as teaching keys. Each
  key folder must carry a one-line README saying the keys are public test keys and must never be
  used for anything real.
- **A9 Axes and conventions.** DN201's z-up axes stay for DN201; the drone courses from DN301 on
  state their convention explicitly and say how it relates to NED. No rewrite to NED is required.
- **A10 Licence.** University lab code is released under the MIT licence; the placeholder
  `LicenseRef-Uni-Lab` becomes `MIT`. The prose is the project's content under the repository's
  terms.

## B. Assessment

- **B1 Gates.** Every mega project uses all five gates R0–R4 (guide 11.6). The checklists and
  signers each handbook proposes are approved. Safety items are never "conditions"; only
  independent reviewers count; the supervisor of real hardware runs signs R3 safety items in
  person.
- **B2 No hardware.** A mega project or course practical may pass without a real-hardware run
  only when the gate record states why and the simulated evidence is complete (MP7's rule,
  extended to all). Motors and propellers never get power before R3 (MP6's stricter rule
  applies to MP6, MP7 and MP8).
- **B3 Rubrics.** Card weights are fixed. Where a card leaves a split open, the handbook's
  proposed split is approved. Default course-project rubric is guide 11.5.
- **B4 Exams.** Every course gets its exams written in this pass (guide 11.4): quizzes come from
  each chapter's "Check yourself" with its answers; midterm (courses of 4+ credits), final (with
  one forensic question) and practical (with a reference solution that is actually run), plus the
  course project brief. Papers are shown in the course section of `UNIVERSITY.html`; keys go in
  the "Answer keys" appendix of the same page.
- **B5 Safeguarding (SE404 peer mentoring).** Minimum rule until an organisation adopts its own
  policy: no one-to-one unsupervised sessions with a minor; sessions happen in a shared or
  recorded space with a responsible adult informed; no private contact details exchanged; any
  concern goes to the responsible adult the same day.
- **B6 L0–L1 grading.** Courses at level L0–L1 use the L0 option of guide 11.4: quizzes,
  practical and course project are graded (quizzes 20 %, practical 50 %, project 30 %, an
  exercise value under A4) with encouraging written feedback. Their midterm and final are still
  written (B4) but are ungraded practice papers. All other courses use the 11.4 default weights.
- **B7 Practical evidence.** A practical's reference solution and hidden evidence live in
  `labs/<C>-P/`; candidates are given only the starting file(s) the paper names. Each `-P`
  README says so.

## C. Verification

- **C1 Sources.** Internet access is now available through the web tools. Every unit is verified
  against the real documents: the verifier opens each cited source, writes the chapter's source
  dossier (guide 3.2, AH-8), checks each tagged claim (FC-1 to FC-15), and either confirms it,
  corrects it, or moves it into an "unverified" box. "Title only, not opened" wording is removed
  wherever the source was opened.
- **C2 New sources.** The sources units proposed adding (textbooks, standards, papers, tool
  documentation) are approved for the registry when the verifier could open the document or a
  reliable official description of it. Paywalled standards are cited by their official
  catalogue page (title, number, edition, year) and any claim about their content stays in an
  "unverified" box unless a freely readable official source confirms it.
- **C3 Untested on hardware.** The owner approves publishing chapters whose hardware steps are
  untested, on condition that each such step is marked "untested on hardware" and says what
  would be needed to test it. Nothing may be claimed as observed on hardware that was not.
- **C4 Status.** A chapter whose gates G1 to G8 pass in the verification pass is "internally
  checked". No chapter is called "published" while it has open hardware items; the catalogue
  shows "internally checked · hardware steps untested" for those.

## D. Kit and platforms

- **D1 Reference kit.** The reference kit is chosen and checked in
  [`build/KIT.md`](build/KIT.md) (written in this pass). Units name kit items from that file; Part B
  hardware steps refer to it.
- **D2 Simulators and stacks.** PX4 is the primary drone stack (ArduPilot is taught as the second
  stack); Gazebo (the current Gazebo release named in KIT.md) is the simulator for robots and
  drones; ROS 2 uses the LTS distribution named in KIT.md. Exact versions are in KIT.md.
- **D3 Linux robot images.** Buildroot is the default image builder for robot computers; Yocto is
  mentioned as the alternative. The bare-metal QEMU "robot OS image" in RB403 stays as the
  teaching image, and the chapter states that a Linux image is the real-world target.
- **D4 E-stop default.** The robot image refuses to arm while the e-stop reads pressed (RB403's
  proposed fix becomes the default behaviour of the teaching code).
