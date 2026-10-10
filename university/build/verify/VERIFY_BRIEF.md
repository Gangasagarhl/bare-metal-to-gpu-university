# Verification pass brief (one unit: a course, a bridge or a mega project)

You are the Source Researcher, Fact-Checker, Diagram reviewer, Editor and Accessibility reviewer
for ONE unit of the university in the git repo at /home/user/bare-metal-to-gpu-university.
Work only in that checkout. Do not commit or push. Never call any mcp__hearthbot__ tool.
Touch only: your unit's chapter folder `university/chapters/<UNIT>/`, the lab folders of your
unit's chapters `university/labs/<CHAPTER>/`, `university/_dossiers/<CHAPTER>.dossier.html` and
`university/qa/<CHAPTER>.json` for your chapters. Do not edit other units' files or build.py.

## Read first
1. `university/OWNER_RULINGS.md` — the owner's rulings. They are binding.
2. `university/build/KIT.md` if it exists (the reference hardware kit).
3. `university/build/AUTHOR_BRIEF.md`, `university/build/AUTHOR_BRIEF_UPPER.md`,
   `university/chapters/FRAGMENT_FORMAT.md`.
4. In `UNIVERSITY_AUTHORING_GUIDE.html` (large: extract text with a small python script that
   strips tags): section 3 (anti-hallucination protocol, AH rules, dossier 3.2, claim tags 3.3,
   unverified boxes), section 9.3 (diagram checklist), section 12 (pipeline, gates G1–G9, the
   Fact-Checker's FC-1..FC-15 checks, QA record), section 13 (style, reading level, accessibility).
5. Your unit's `NOTES.md` (it lists the unverified claims and the open owner decisions).

## Web access
The bash shell has NO general internet (only package indexes and GitHub). Load the web tools with
ToolSearch "select:WebSearch,WebFetch" first.
- **WebSearch** (mode "standard"; "extended" when the standard result is thin) works for every
  topic. Its result quotes and summarises the pages it found, with their URLs. Use it to locate the
  official document and to confirm what it says.
- **WebFetch** reaches ONLY `github.com` and `raw.githubusercontent.com` in this session; the
  environment's network policy denies every other host (the call fails with ENOTFOUND or 403).
  Many standards and manuals have their source on GitHub (RISC-V ISA manual, the C++ draft
  `cplusplus/draft`, Linux `Documentation/` via `torvalds/linux`, VIRTIO `oasis-tcs/virtio-spec`,
  LLVM `llvm/llvm-project` docs, Zephyr, PX4, ArduPilot, ROS 2, Gazebo, CUDA samples, QEMU docs
  source). Open those there and record the GitHub URL and the commit/tag you read.
- Do not retry a denied host; do not pretend a document was opened.
Prefer tier-1 sources: the vendor's or project's own documentation, the standard's official page,
the paper itself, the book publisher's page.

**Honest recording (build-lead ruling for this pass, under owner ruling C1):**
- A document you opened with WebFetch: "how accessed" = the URL opened; tier as usual.
- A document you could not open but whose official page a WebSearch result quoted: "how accessed"
  = "search excerpt of <URL> (WebSearch; document not opened: network policy)". A claim that the
  quoted official text confirms counts as confirmed; cite the section the excerpt names. Say in
  the claim's dossier F-row that the evidence is a search excerpt.
- A claim confirmed only by secondary pages (blogs, forums, vendor summaries) stays in an
  "unverified" box with the reason "official text not opened in this pass".
- Record every URL you actually opened or quoted; never invent a section number.

## For each chapter of the unit
1. **Dossier (G1).** Write `university/_dossiers/<CHAPTER>.dossier.html` as an HTML fragment
   (same rules as chapters: no `<script>`, every id prefixed `<CHAPTER>-dos-`; URLs are allowed
   here only as plain text inside `<code>`, never as links). Start it with
   `<!--meta\nid: <CHAPTER>-dossier\nchapter: <CHAPTER>\n-->`. Contents, per AH-8..AH-14:
   a documents table with one row per source D-id the chapter cites (title exactly as printed,
   issuing body/authors, version or edition as shown or "no version shown", sections used, date
   accessed 2026-10-10, how accessed = the URL opened, tier); a facts list (F-ids) keyed to the
   chapter's tagged claims; a "not found" list; and the toolchain versions from
   `university/labs/TOOLCHAIN_VERSIONS.txt`. Lab-run sources (R-ids) are listed as local evidence.
2. **Fact-check (G6).** Check every tagged technical claim and every "Not verified"/"unverified"
   box against the opened sources (FC-1..FC-15). For each:
   - confirmed → keep it, cite the real section in the claim tag and in Sources; if it was in an
     unverified box, move the text out of the box;
   - wrong → correct the chapter text (and the code, inputs and expected outputs if the error
     reaches the lab; then re-run that lab with `university/labs/run_lab.sh university/labs/<CH>`
     and update the prose that quotes its output);
   - cannot be confirmed from an openable source → keep or put it in an unverified box with an
     honest reason.
   Update the chapter's Sources section: remove "title only — not opened" wording for sources you
   opened; give edition/version and section as printed. Keep chapters free of URLs.
   Never claim hardware observations; "untested on hardware" boxes stay (owner ruling C3) but
   each must say what would be needed to test it.
3. **Diagrams (G4).** Check every inline SVG against guide 9.3 (labels readable, title/desc for
   accessibility, colours not the only carrier of meaning, matches the text). Fix small issues.
4. **Edit (G7).** Fix language errors, broken internal references, and reading-level problems
   for the chapter's level (guide 13). Do not trim for length (ruling A1).
5. **Accessibility (G8).** Every figure has a text alternative; tables have header cells;
   headings are in order; no meaning carried only by colour.
6. **Labs (G5).** Re-run every lab of your chapters with `run_lab.sh`. If outputs differ only by
   run-to-run timing noise, restore the recorded files with `git checkout -- university/labs/<CH>`
   (ruling A5). Any real failure must be fixed.
7. **QA record.** Write `university/qa/<CHAPTER>.json`:
   ```
   {"dossier": "approved — <n> documents opened, <m> facts, <k> not found",
    "checker": "Fact-Checker agent (verification pass 2026-10-10)",
    "factcheck": "done",
    "factcheck_note": "ok — <x> claims checked, <y> corrected, <z> left unverified",
    "diagrams": "ok — <short>", "edit": "ok — <short>", "accessibility": "ok — <short>",
    "findings": ["one line per correction or remaining unverified item"],
    "corrections": [{"where": "<section id>", "was": "<short>", "now": "<short>", "source": "D3 §x"}],
    "hardware_untested": ["one line per untested-on-hardware step"]}
   ```
   Set "factcheck" to "done" only when every claim was dealt with.

## For the unit
- Apply `OWNER_RULINGS.md` to every open decision in your `NOTES.md`; add a section
  "## Owner rulings applied" listing each decision and its ruling (cite the ruling id, or say
  "decided by verifier:" and the decision). Make the content changes those rulings require
  (e.g. ruling A8 key READMEs, A10 licence, D4 e-stop default) inside your unit's files only.
- Add a section "## Verification pass" to `NOTES.md` summarising what was opened, corrected and
  left unverified.
- Validate every fragment you changed (html.parser balance, ids prefixed, no URLs in chapters,
  no `<script>`), then run `python3 university/build/build.py` and fix any PROBLEM line that
  mentions your unit, chapters or dossiers. Other units' problems are not yours.
  build.py rewrites `university/UNIVERSITY.html`; restore it afterwards with
  `git checkout -- university/UNIVERSITY.html`.

## Report
Reply with a short summary: documents opened, claims checked/corrected/left unverified, the most
important corrections, labs re-run (pass/expected-fail/untested on hardware), rulings applied,
anything still open.
