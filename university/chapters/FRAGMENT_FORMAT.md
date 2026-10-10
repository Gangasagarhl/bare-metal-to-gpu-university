# Chapter fragment format

Every chapter is one file: `university/chapters/<COURSE>/<CHAPTER-ID>.html`
(for example `university/chapters/KID101/F0-01.html`). The build script
(`university/build/build.py`) wraps each fragment with the chapter's header and
front-matter card and places it inside the single file `university/UNIVERSITY.html`.

Because the whole university is ONE html page, every `id` must be unique across
the page. **Prefix every id you create with the chapter id** (`F0-01-hook`,
`F0-01-fig1`, `F0-01-src-D1`). No JavaScript, no event handlers, no external
assets, no URLs (guide AH-30, 13.7).

## 1. Metadata comment (first thing in the file)

```
<!--meta
id: F0-01
title: Machines that follow recipes
course: KID101
level: L0
prereqs: none
time: 2–3 sessions of 20–30 minutes
hardware: none
maps: Story-level preparation for HW101–HW102; no curriculum milestone
analogy: F0 — everyday life at home
-->
```

`prereqs` may contain chapter ids and course codes; the builder turns ids such as
`F0-02` or `KID101` into links.

## 2. Body: the 21 template sections, in this exact order (guide 7.3)

Use `<h2>` for each section with exactly these headings and ids
(`<ID>` = chapter id):

| # | heading | id |
|---|---|---|
| 1 | Story hook | `<ID>-hook` |
| 2 | What you will be able to do | `<ID>-able` |
| 3 | Jargon box | (a `div.co.jargon` with `id="<ID>-jargon"`, no h2) |
| 4 | The idea in pictures | `<ID>-pictures` |
| 5 | Layer 1 — the kid explanation | `<ID>-layer1` |
| 6 | Layer 2 — the student explanation | `<ID>-layer2` |
| 7 | Layer 3 — the engineer's depth | `<ID>-layer3` |
| 8 | How the hardware actually does it | `<ID>-hardware` |
| 9 | Worked example | `<ID>-worked` |
| 10 | Code walk-through | `<ID>-code` |
| 11 | Where the analogy breaks | `<ID>-breaks` |
| 12 | Common mistakes and debugging | `<ID>-mistakes` |
| 13 | Transition box | (a `div.co.transition` with `id="<ID>-transition"`, no h2) |
| 14 | Check yourself | `<ID>-check` |
| 15 | Lab | `<ID>-lab` |
| 16 | Forensic lab | `<ID>-forensic` |
| 17 | Mini-project | `<ID>-project` |
| 18 | Summary | `<ID>-summary` |
| 19 | Sources | `<ID>-sources` |
| 20 | Glossary links | `<ID>-glossary` |
| 21 | Next chapter | `<ID>-next` |
| — | Answers to Check yourself | `<ID>-answers` |

Use `<h3>` inside sections. A section that does not apply says so in one sentence
and points to where the skill is practised (guide 7: never omitted).

## 3. Callouts — only these seven classes (guide 7.6)

```
<div class="co note"><span class="lbl">Note</span><p>…</p></div>
<div class="co analogy"><span class="lbl">Analogy</span><p>…</p></div>
<div class="co jargon" id="<ID>-jargon"><span class="lbl">Jargon box</span><dl class="gl">
   <dt>Bit</dt><dd><b>Simple words:</b> … <b>Analogy:</b> … <b>Precise:</b> … <sup class="src"><a href="#<ID>-src-D1">D1</a></sup></dd>
</dl></div>
<div class="co warning"><span class="lbl">Watch out</span><p>…</p></div>
<div class="co safety"><span class="lbl">Safety</span><p>…</p></div>
<div class="co unverified"><span class="lbl">Not verified — check before relying on this</span><p>…</p></div>
<div class="co transition" id="<ID>-transition"><span class="lbl">Transition box</span>
   <p><b>Where you came from:</b> … <b>What carries over:</b> … <b>What changes:</b> … <b>What comes next:</b> …</p></div>
```

## 4. Claim tags (AH-15)

`<sup class="src"><a href="#<ID>-src-D1">D1</a></sup>` after a technical statement.
L0–L1 chapters may group tags at the end of a paragraph.

## 5. Figures (guide 9)

```
<figure id="<ID>-fig1"><div class="svgbox">
<svg viewBox="0 0 760 260" role="img" aria-labelledby="<ID>-f1t <ID>-f1d">
<title id="<ID>-f1t">…</title><desc id="<ID>-f1d">… full text alternative …</desc>
… only classes sv-box sv-b1..sv-b8 sv-dot1..4 sv-line sv-line2 sv-dash sv-head sv-t sv-ts sv-tb sv-tm …
</svg></div><figcaption><b>Figure 1.</b> …</figcaption></figure>
```

No colours written as attributes; no `<style>` inside SVG. Text at least 11px.
Arrowheads: draw them as small `<polygon class="sv-head">` (no `<marker>` ids that could clash).

## 6. Code listings and real output (AH-24, AH-25, 13.4)

Never paste code or output by hand. Put the source in
`university/labs/<ID>/<name>.cpp`, run `university/labs/run_lab.sh university/labs/<ID>`,
and reference the files; the builder copies them in byte for byte:

```
<p><b>Listing 1</b> — <code>hello.cpp</code></p>
<pre class="listing" data-src="F0-29/hello.cpp"></pre>
<p><b>Build command</b> (the one actually used): …</p>
<pre class="output" data-run="F0-29/hello"></pre>     ← inserts hello.out, labelled with hello.log
```

`data-run` inserts the real output plus a one-line record (toolchain, date, exit code) from `<name>.log`.
A program that reads input gets `<name>.in`. A listing that must fail to compile
(for lessons about compiler errors) gets an empty `<name>.expect-fail` file.
Every listing is followed by a line-by-line table (line, code, what it does, why it matters).

## 7. Glossary proposals

Each course folder has `glossary.json`: a list of entries in the four-part format
of guide 10.2:

```
[{"term": "Bit", "simple": "…", "analogy": "…", "precise": "…",
  "source": "D1 of F0-02 (pending verification)", "related": ["Byte"],
  "chapters": ["F0-02", "F0-03"]}]
```

Glossary anchors in the page are `gl-<term in lower case, non-letters replaced by ->`,
for example `#gl-bit`, `#gl-kernel-gpu` (no trailing dash). Link terms in "Glossary links" this way.

## 8. Links

Chapters: `#F0-02`. Course cards: `#KID101`. Faculties: `#fac-F0`. Analogy registry: `#analogies`.
Safety: `#safety`. Curriculum: `../SYSTEMS_CURRICULUM.html#trackE` (relative to `university/`).
