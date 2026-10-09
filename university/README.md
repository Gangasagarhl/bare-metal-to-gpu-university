# Bare Metal to GPU University (one-file edition)

**Open [`UNIVERSITY.html`](UNIVERSITY.html)**. That one self-contained page holds the whole
university: the home page, levels, goal paths, the catalogue of all 603 chapters, safety rules,
the analogy registry, the glossary, all 13 faculties with their 87 course cards and every written
chapter, the 10 bridge chapters, the 8 mega projects and a QA record for each written chapter.
It has no JavaScript and no external assets, and it works offline in light and dark mode.

It is built by following the three HTML prompt files at the repository root:

| File | Role |
|---|---|
| `UNIVERSITY_AUTHORING_GUIDE.html` | The instruction set: blueprint, chapter template, anti-hallucination protocol, style |
| `SYSTEMS_CURRICULUM.html` | The technical backbone: tracks, milestones, glossary seed |
| `DRIVERS.html` | The Windows-drivers learning page where the forensic-lab idea comes from |

## Folder layout

```
university/
├─ UNIVERSITY.html          ← the generated one-file university (open this)
├─ README.md                ← this file
├─ build/
│  ├─ build.py              ← Integrator: assembles UNIVERSITY.html and checks links/ids/HTML
│  ├─ AUTHOR_BRIEF*.md      ← the exact briefs the chapter-author agents received
│  ├─ prompts/<COURSE>.txt  ← the full task given to each course's author agent
│  ├─ QUEUE.txt, PROGRESS.md← build order of all 87 courses and what is done so far
│  └─ RESUME.md, resume.sh, commit_course.sh ← how to continue after an interruption
├─ chapters/
│  ├─ FRAGMENT_FORMAT.md    ← the chapter file format (21 template sections)
│  └─ <COURSE>/             ← one folder per course, e.g. KID101/
│     ├─ <CHAPTER-ID>.html  ← one chapter fragment, e.g. F0-01.html
│     ├─ glossary.json      ← glossary entries proposed by the course (four-part format)
│     └─ NOTES.md           ← unverified claims, open decisions, listings run
├─ labs/
│  ├─ run_lab.sh            ← Lab Engineer runner: builds and runs every listing of a chapter
│  ├─ setup_toolchains.sh   ← what was installed in the build container
│  ├─ TOOLCHAIN_VERSIONS.txt← the versions as printed by the tools
│  └─ <CHAPTER-ID>/         ← listings (.cpp/.cu/.hip/...), inputs (.in), real outputs (.out), run records (.log)
└─ qa/                      ← fact-check results per chapter (<CHAPTER-ID>.json), when done
```

## Rebuild

```
university/labs/run_lab.sh university/labs/F0-29     # re-run one chapter's listings
python3 university/build/build.py --check            # regenerate UNIVERSITY.html; fails on any problem
```

## Build progress

The build is in progress. [`build/PROGRESS.md`](build/PROGRESS.md) lists every course as done or
pending and is updated with each course commit; [`build/RESUME.md`](build/RESUME.md) says how to
continue after an interruption.

## Status and honesty

- Chapters are **drafts (v0.1)**. The build container had **no internet access**, so no
  official document or textbook could be opened. Following the guide's anti-hallucination
  protocol, sources are named by title and marked *pending verification*, the dossier gate
  (G1) is open, and nothing is labelled "published".
- **Every code listing was really compiled and run** in the build container, and its output
  is copied byte for byte into the page. GPU code is compiled with the real CUDA (nvcc) and
  HIP (hipcc) compilers, but the container has no GPU, so those runs show the runtime's own
  "no device" error and are marked *untested on hardware*.
- The catalogue in `UNIVERSITY.html` shows the status of every chapter.
