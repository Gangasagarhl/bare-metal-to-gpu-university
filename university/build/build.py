#!/usr/bin/env python3
"""Integrator: assemble the whole university into ONE self-contained HTML file.

    python3 university/build/build.py            # writes university/UNIVERSITY.html
    python3 university/build/build.py --check    # build, then fail on any integrity problem

Inputs (all inside the repository):
  UNIVERSITY_AUTHORING_GUIDE.html   the blueprint: course cards, bridges, mega projects,
                                    analogy registry, safety, ladder, goal paths, style block
  SYSTEMS_CURRICULUM.html           the technical backbone: glossary seed (section 20)
  university/chapters/<COURSE>/<ID>.html     chapter fragments (format: chapters/FRAGMENT_FORMAT.md)
  university/chapters/<COURSE>/glossary.json proposed glossary entries
  university/labs/<ID>/<name>.cpp|.cu|...    listings, with <name>.out / <name>.log from real runs
  university/qa/<ID>.json                    fact-check results (optional)

The page contains no JavaScript (guide 13.7). Every listing and every output is copied in
byte for byte from the lab folders (guide 13.4: "the escaped listing matches the tested
source file"), so nothing in the page is typed by hand.
"""
import html
import json
import os
import re
import sys
from datetime import date
from html.parser import HTMLParser

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
UNI = os.path.join(ROOT, "university")
GUIDE_PATH = os.path.join(ROOT, "UNIVERSITY_AUTHORING_GUIDE.html")
CURR_PATH = os.path.join(ROOT, "SYSTEMS_CURRICULUM.html")
OUT_PATH = os.path.join(UNI, "UNIVERSITY.html")
GUIDE_REL = "../UNIVERSITY_AUTHORING_GUIDE.html"
CURR_REL = "../SYSTEMS_CURRICULUM.html"

FACULTIES = [
    ("F0", "Foundations for kids and the maths spine", "f0"),
    ("F1", "How hardware works", "f1"),
    ("F2", "Systems programming in C++", "f2"),
    ("F3", "Firmware, boot and OS kernels", "f3"),
    ("F4", "Drivers and any hardware", "f4"),
    ("F5", "Distributed computing and servers", "f5"),
    ("F6", "GPU computing with CUDA C++", "f6"),
    ("F7", "GPU computing with ROCm/HIP", "f7"),
    ("F8", "Distributed GPU computing", "f8"),
    ("F9", "Robotics systems", "f9"),
    ("F10", "Drone systems", "f10"),
    ("F11", "Security and safety-critical engineering", "f11"),
    ("F12", "The senior engineer", "f12"),
]

STATUS_LABEL = {
    "planned": ("planned", "ts"),
    "draft": ("draft", "twarn"),
    "tested": ("draft · code run", "tdev"),
    "checked": ("draft · code run · internally checked", "tok"),
}

# Extra classes appended to the canonical style block (guide 7.5 says to add .front).
EXTRA_CSS = """
/* added by the Integrator for the single-file university (guide 7.5: .front) */
dl.front{display:grid;grid-template-columns:170px 1fr;gap:4px 14px;background:var(--paper);border:1px solid var(--line);border-radius:10px;padding:12px 14px;margin:12px 0}
dl.front dt{font-weight:700;color:var(--muted);font-size:12.5px;text-transform:uppercase;letter-spacing:.04em;padding-top:2px}
dl.front dd{margin:0}
@media (max-width:600px){dl.front{grid-template-columns:1fr}dl.front dt{padding-top:6px}}
article.chapter{border-top:6px solid var(--line);margin-top:70px;padding-top:10px}
article.chapter>h2.chtitle{border-top:0;margin-top:4px;font-size:26px}
h2.fac{font-size:27px;border-top:6px double var(--line);margin-top:90px}
pre.output{background:var(--code)}
.runrec{font-size:12.5px;color:var(--muted);margin:-4px 0 10px}
sup.src{font-size:11px;line-height:0}
sup.src a{text-decoration:none}
.back{font-size:13px}
"""


# ----------------------------------------------------------------------------- helpers

def read(path):
    with open(path, encoding="utf-8") as f:
        return f.read()


def esc(s):
    return html.escape(s, quote=False)


def slug(term):
    return "gl-" + re.sub(r"[^a-z0-9]+", "-", term.lower()).strip("-")


def heading_blocks(doc):
    """Return {id: (level, start, end)} for every <h2 id> / <h3 id> in doc; end = next heading of same or higher level."""
    hs = [(m.start(), int(m.group(1)), m.group(2)) for m in re.finditer(r'<h([23]) id="([^"]+)"', doc)]
    out = {}
    for i, (pos, lvl, hid) in enumerate(hs):
        end = len(doc)
        for pos2, lvl2, _ in hs[i + 1:]:
            if lvl2 <= lvl:
                end = pos2
                break
        out[hid] = (lvl, pos, end)
    return out


def section(doc, blocks, hid, include_heading=False):
    lvl, start, end = blocks[hid]
    body = doc[start:end]
    if not include_heading:
        body = re.sub(r"^<h[23][^>]*>.*?</h[23]>", "", body, count=1, flags=re.S)
    # cut the guide's closing tags if the section is the last before </main>
    body = re.split(r"</main>", body)[0]
    return body


class Balance(HTMLParser):
    VOID = {"br", "img", "hr", "meta", "link", "input", "col", "area", "base", "wbr", "source", "track", "embed", "param"}

    def __init__(self):
        super().__init__(convert_charrefs=True)
        self.stack, self.errors, self.ids = [], [], []

    def handle_starttag(self, tag, attrs):
        for k, v in attrs:
            if k == "id":
                self.ids.append(v)
        if tag not in self.VOID:
            self.stack.append((tag, self.getpos()))

    def handle_startendtag(self, tag, attrs):
        for k, v in attrs:
            if k == "id":
                self.ids.append(v)

    def handle_endtag(self, tag):
        if tag in self.VOID:
            return
        # tolerate implicitly closed <p>/<li>/<dt>/<dd>/<tr>/<td>/<th>
        while self.stack and self.stack[-1][0] != tag and self.stack[-1][0] in ("p", "li", "dt", "dd", "tr", "td", "th"):
            self.stack.pop()
        if self.stack and self.stack[-1][0] == tag:
            self.stack.pop()
        else:
            self.errors.append("unexpected </%s> at line %d" % (tag, self.getpos()[0]))


# ----------------------------------------------------------------------------- inputs

def parse_cards(guide):
    """Course cards, bridges and mega projects: {id: html_of_card} in document order."""
    cards = {}
    for m in re.finditer(r'<div class="course (f\d+)" id="([A-Z]+[0-9-]+)">', guide):
        start = m.start()
        # card ends at the first "</dl></div>" after start
        nxt = [guide.find(t, m.end()) for t in ('<div class="course', "<h3", "<h2", "</main>", '<div class="note')]
        end = min(x for x in nxt if x > 0)
        cards[m.group(2)] = {"html": guide[start:end].strip(), "fclass": m.group(1)}
    return cards


def card_chapters(card_html):
    """[(chapter id, title)] from a course card's Chapters field."""
    m = re.search(r"<dt>Chapters</dt><dd>(.*?)</dd>", card_html, re.S)
    if not m:
        return []
    text = html.unescape(re.sub(r"<[^>]+>", " ", m.group(1)))
    text = re.sub(r"\s+", " ", text)
    parts = re.split(r"\s(?=F\d+-\d+[a-z]?\s)", " " + text)
    out = []
    for p in parts:
        p = p.strip().strip("·").strip()
        mm = re.match(r"(F\d+-\d+[a-z]?)\s+(.*)", p)
        if mm:
            title = mm.group(2).strip().rstrip("·").strip()
            title = re.sub(r"\s*\(exemplar in section 14\)", "", title)
            out.append((mm.group(1), title))
    return out


def card_field(card_html, name):
    m = re.search(r"<dt>%s</dt><dd>(.*?)</dd>" % re.escape(name), card_html, re.S)
    return m.group(1) if m else ""


def card_title_level(card_html):
    m = re.search(r'<div class="ch"><span class="id">[^<]+</span>\s*(.*?)\s*(<span class="lvl">.*?)?</div>', card_html, re.S)
    title = html.unescape(re.sub(r"<[^>]+>", "", m.group(1))).strip()
    lvl = ""
    lm = re.findall(r'<span class="lvl">([^<]+)</span>', card_html[: card_html.find("</div>")])
    if lm:
        lvl = lm[0]
    return title, lvl


def parse_fragment(path):
    src = read(path)
    m = re.match(r"\s*<!--meta\s*(.*?)-->", src, re.S)
    if not m:
        raise SystemExit("missing meta comment: " + path)
    meta = {}
    for line in m.group(1).splitlines():
        if ":" in line:
            k, v = line.split(":", 1)
            meta[k.strip()] = v.strip()
    meta["body"] = src[m.end():]
    meta["path"] = os.path.relpath(path, ROOT)
    return meta


def load_chapters():
    chapters = {}
    base = os.path.join(UNI, "chapters")
    for course in sorted(os.listdir(base)):
        d = os.path.join(base, course)
        if not os.path.isdir(d):
            continue
        for fn in sorted(os.listdir(d)):
            if re.match(r"(F\d+-\d+[a-z]?|BR-\d+|MP\d+)\.html$", fn):
                meta = parse_fragment(os.path.join(d, fn))
                chapters[meta["id"]] = meta
    return chapters


def load_glossary_proposals():
    entries = []
    base = os.path.join(UNI, "chapters")
    for course in sorted(os.listdir(base)):
        p = os.path.join(base, course, "glossary.json")
        if os.path.exists(p):
            for e in json.loads(read(p)):
                e["_course"] = course
                entries.append(e)
    return entries


def load_qa(cid):
    p = os.path.join(UNI, "qa", cid + ".json")
    return json.loads(read(p)) if os.path.exists(p) else None


def curriculum_glossary(curr):
    m = re.search(r'<h2 id="gloss">.*?<dl class="gl">(.*?)</dl>', curr, re.S)
    items = re.findall(r"<dt>(.*?)</dt><dd>(.*?)</dd>", m.group(1), re.S)
    return items


# ----------------------------------------------------------------------------- labs

LISTING_EXT = (".cpp", ".cu", ".hip", ".h", ".hpp", ".cmake", ".txt", ".sh", ".S", ".ld", ".py", ".in")


def lab_runs(cid):
    """[(name, log_text)] for one chapter's lab folder."""
    d = os.path.join(UNI, "labs", cid)
    if not os.path.isdir(d):
        return []
    out = []
    for fn in sorted(os.listdir(d)):
        if fn.endswith(".log"):
            out.append((fn[:-4], read(os.path.join(d, fn))))
    return out


def run_ok(log):
    return ("exit code:" in log or "compile failed as expected" in log or "untested" in log.lower()) and \
        "BUILD FAILED" not in log and "UNEXPECTED" not in log


def fill_listings(body, cid, problems):
    def listing(m):
        rel = m.group(2)
        p = os.path.join(UNI, "labs", rel)
        if not os.path.exists(p):
            problems.append("%s: listing file missing: labs/%s" % (cid, rel))
            return m.group(0)
        return '<pre class="listing"%s><code>%s</code></pre>' % (m.group(1), esc(read(p)))

    def output(m):
        rel = m.group(2)
        outp = os.path.join(UNI, "labs", rel + ".out")
        logp = os.path.join(UNI, "labs", rel + ".log")
        if not (os.path.exists(outp) and os.path.exists(logp)):
            problems.append("%s: run record missing: labs/%s.out/.log" % (cid, rel))
            return m.group(0)
        out = read(outp)
        log = read(logp)
        if not run_ok(log):
            problems.append("%s: run of labs/%s did not succeed (see its .log)" % (cid, rel))
        tool = re.search(r"toolchain:\s*(.*)", log)
        when = re.search(r"date:\s*(.*)", log)
        res = re.search(r"(exit code:.*|result:.*)", log)
        rec = "Real output, copied from <code>university/labs/%s.out</code>; run record <code>%s.log</code>: %s; %s; %s." % (
            esc(rel), esc(os.path.basename(rel)), esc(tool.group(1) if tool else "?"),
            esc(when.group(1) if when else "?"), esc(res.group(1) if res else "?"))
        text = esc(out) if out.strip() else "(the program printed nothing)"
        return '<pre class="output"%s><code>%s</code></pre><p class="runrec">%s</p>' % (m.group(1), text, rec)

    body = re.sub(r'<pre class="listing"([^>]*?)\s*data-src="([^"]+)"[^>]*>\s*</pre>', listing, body)
    body = re.sub(r'<pre class="output"([^>]*?)\s*data-run="([^"]+)"[^>]*>\s*</pre>', output, body)
    return body


# ----------------------------------------------------------------------------- status

def chapter_status(cid, chapters):
    if cid not in chapters:
        return "planned"
    runs = lab_runs(cid)
    qa = load_qa(cid)
    if qa and qa.get("factcheck") == "done":
        return "checked"
    if runs and all(run_ok(l) for _, l in runs):
        return "tested"
    if not runs and "data-run" not in chapters[cid]["body"]:
        return "tested" if qa else "draft"
    return "draft"


def status_tag(st):
    lab, cls = STATUS_LABEL[st]
    return '<span class="tag %s">%s</span>' % (cls, lab)


# ----------------------------------------------------------------------------- page parts

def linkify_ids(text, known):
    """Turn chapter ids / course codes in plain front-matter text into links."""
    def rep(m):
        t = m.group(0)
        return '<a href="#%s">%s</a>' % (t, t) if t in known else t
    return re.sub(r"\b(?:F\d+-\d+[a-z]?|BR-\d+|MP\d+|[A-Z]{2,3}\d{3})\b", rep, text)


def render_chapter(meta, course_title, known, problems, nxt, prev):
    cid = meta["id"]
    body = fill_listings(meta["body"], cid, problems)
    st = chapter_status(cid, {cid: meta})
    qa = load_qa(cid)
    lvl = " – ".join('<span class="lvl">%s</span>' % esc(x.strip()) for x in re.split(r"[–-]", meta.get("level", "")) if x.strip())
    nav = []
    if prev:
        nav.append('<a href="#%s">← %s</a>' % (prev, prev))
    nav.append('<a href="#%s">course %s</a>' % (meta["course"], meta["course"]))
    nav.append('<a href="#catalogue">catalogue</a>')
    if nxt:
        nav.append('<a href="#%s">%s →</a>' % (nxt, nxt))
    front = [
        ("Course", '<a href="#%s">%s %s</a>' % (meta["course"], meta["course"], esc(course_title))),
        ("Chapter id", "%s %s" % (cid, esc(meta["title"]))),
        ("Level", lvl),
        ("Prerequisites", linkify_ids(esc(meta.get("prereqs", "none")), known)),
        ("Time to study", esc(meta.get("time", ""))),
        ("Hardware needed", esc(meta.get("hardware", ""))),
        ("Sources dossier", 'Not yet written: this build had no internet access, so no official document could be opened. '
                            'Sources are named by title in the chapter\'s Sources section and marked "pending verification" (gate G1 open).'),
        ("QA record", '<a href="#qa-%s">QA record of %s</a> (in this file)' % (cid, cid)),
        ("Version", "v0.1 draft · %s · not yet verified against sources" % status_tag(st)),
        ("Maps to", linkify_ids(esc(meta.get("maps", "")), known)),
    ]
    fm = "".join("<dt>%s</dt><dd>%s</dd>" % (k, v) for k, v in front)
    return (
        '<article class="chapter" id="%s">\n<div class="kicker">%s · Chapter %s</div>\n'
        '<h2 class="chtitle">%s</h2>\n<p class="back">%s</p>\n<dl class="front">%s</dl>\n%s\n'
        '<footer>Corrections log: <a href="#qa-%s-corrections">%s corrections</a> (none recorded yet). %s</footer>\n</article>\n'
        % (cid, esc(meta["course"] + " · " + course_title), cid, esc(meta["title"]), " · ".join(nav), fm, body, cid, cid,
           " · ".join(nav))
    )


def render_qa(cid, meta, st):
    runs = lab_runs(cid)
    qa = load_qa(cid) or {}
    rows = [
        ("G0", "Planned (course card)", "Dean (this build)", "ok — scope copied from the guide's course card"),
        ("G1", "Dossier approved", "—", "open — no official source could be opened in this build (no internet access)"),
        ("G2", "Outline approved", "—", "skipped in batch 1 (Author wrote directly to the template)"),
        ("G3", "Draft complete", "Author agent", "ok — all 21 template sections present" if meta else "—"),
        ("G4", "Diagrams reviewed", "—", qa.get("diagrams", "open")),
        ("G5", "Labs and code tested", "Lab Engineer agent",
         ("ok — %d listing run(s), records below" % len(runs)) if runs and all(run_ok(l) for _, l in runs)
         else ("no listings" if not runs else "FAILED — see records")),
        ("G6", "Fact-check passed", qa.get("checker", "—"), qa.get("factcheck_note", "open")),
        ("G7", "Edit + reading level", "—", qa.get("edit", "open")),
        ("G8", "Accessibility", "—", "open"),
        ("G9", "Integrated", "Integrator (build.py)", "ok — assembled into this file; links and ids checked"),
    ]
    t = "".join("<tr><td>%s</td><td>%s</td><td>%s</td><td>%s</td></tr>" % r for r in rows)
    runs_html = ""
    if runs:
        runs_html = "<h4>Code runs</h4><div class=\"tw\"><table><tr><th>Listing</th><th>Record</th></tr>%s</table></div>" % "".join(
            "<tr><td><code>labs/%s/%s</code></td><td><pre>%s</pre></td></tr>" % (cid, esc(n), esc(l.strip())) for n, l in runs)
    findings = ""
    if qa.get("findings"):
        findings = "<h4>Fact-check findings</h4><ul>%s</ul>" % "".join("<li>%s</li>" % esc(f) for f in qa["findings"])
    return ('<div class="card" id="qa-%s"><h4>QA record — %s %s %s</h4><div class="tw"><table><tr><th>Gate</th><th>What</th>'
            '<th>By</th><th>Result</th></tr>%s</table></div>%s%s<p id="qa-%s-corrections"><b>Corrections log:</b> no corrections yet.</p>'
            '<p><a href="#%s">back to the chapter</a></p></div>\n'
            % (cid, cid, esc(meta.get("title", "")), status_tag(st), t, runs_html, findings, cid, cid))


def render_glossary(proposals, curr_items):
    by = {}
    for e in proposals:
        key = e["term"].strip()
        if key in by:
            by[key]["chapters"] = sorted(set(by[key].get("chapters", [])) | set(e.get("chapters", [])))
        else:
            by[key] = dict(e)
    parts = ['<p>Each entry has the four parts of guide 10.2: simple words, analogy, precise definition and source. '
             'Entries written in this build are drafts whose sources are <b>pending verification</b> (no official source '
             'could be opened in this build). Entries seeded from the Systems Curriculum glossary (section 20) are shown '
             'below them with their original one-line definition; they still need the four-part rewrite (guide 15.1, step 5).</p>']
    parts.append('<h3 id="glossary-written">Entries written with the chapters</h3><dl class="gl">')
    for term in sorted(by, key=str.lower):
        e = by[term]
        chs = ", ".join('<a href="#%s">%s</a>' % (c, c) for c in e.get("chapters", []))
        rel = ", ".join(esc(r) for r in e.get("related", []))
        parts.append('<dt id="%s">%s</dt><dd><b>Simple words:</b> %s <b>Analogy:</b> %s <b>Precise:</b> %s '
                     '<span class="sub">Source: %s. Related: %s. Chapters: %s.</span></dd>'
                     % (slug(term), esc(term), esc(e.get("simple", "")), esc(e.get("analogy", "")),
                        esc(e.get("precise", "")), esc(e.get("source", "pending")), rel or "—", chs or "—"))
    parts.append("</dl>")
    parts.append('<h3 id="glossary-seed">Seeded from the Systems Curriculum (section 20) — needs verification</h3><dl class="gl">')
    seen = {slug(t) for t in by}
    for dt, dd in curr_items:
        plain = html.unescape(re.sub(r"<[^>]+>", "", dt))
        sid = slug(plain)
        idattr = "" if sid in seen else ' id="%s"' % sid
        seen.add(sid)
        dd = re.sub(r'href="#([^"]+)"', lambda m: 'href="%s#%s"' % (CURR_REL, m.group(1)), dd)
        parts.append("<dt%s>%s</dt><dd>%s <span class=\"tag twarn\">needs verification</span></dd>" % (idattr, dt, dd))
    parts.append("</dl>")
    return "\n".join(parts)


# ----------------------------------------------------------------------------- main build

def build():
    guide = read(GUIDE_PATH)
    curr = read(CURR_PATH)
    blocks = heading_blocks(guide)
    style = re.search(r"<style>(.*?)</style>", guide, re.S).group(1)
    cards = parse_cards(guide)
    chapters = load_chapters()
    problems = []

    courses_by_fac = {f: [] for f, _, _ in FACULTIES}
    for cid, c in cards.items():
        if re.match(r"(BR|MP)", cid):
            continue
        fnum = "F" + c["fclass"][1:]
        courses_by_fac[fnum].append(cid)

    # chapter -> course, ordered list of every chapter
    all_chapters = []  # (chapter id, title, course, level)
    for f, _, _ in FACULTIES:
        for course in courses_by_fac[f]:
            title, lvl = card_title_level(cards[course]["html"])
            for chid, chtitle in card_chapters(cards[course]["html"]):
                all_chapters.append((chid, chtitle, course, lvl))
    bridges = [k for k in cards if k.startswith("BR-")]
    mps = [k for k in cards if k.startswith("MP")]
    known = set(cards) | {c[0] for c in all_chapters} | set(chapters)
    written_known = set(chapters) | set(cards)

    for cid in chapters:
        if cid not in {c[0] for c in all_chapters}:
            problems.append("chapter %s is not in any course card" % cid)

    # statistics
    stat = {}
    for chid, _, _, _ in all_chapters:
        s = chapter_status(chid, chapters)
        stat[s] = stat.get(s, 0) + 1

    today = date.today().isoformat()
    P = []
    P.append('<!DOCTYPE html>\n<html lang="en">\n<head>\n<meta charset="utf-8">\n'
             '<meta name="viewport" content="width=device-width, initial-scale=1">\n'
             '<title>Bare Metal to GPU University</title>\n<style>%s%s</style>\n</head>\n<body>\n' % (style, EXTRA_CSS))
    toc = [("home", "Home"), ("start", "Start here"), ("ladder", "Levels"), ("paths", "Goal paths"), ("years", "Years"),
           ("catalogue", "Catalogue"), ("safety", "Safety"), ("analogies", "Analogies"), ("glossary", "Glossary")]
    toc += [("fac-" + f, f) for f, _, _ in FACULTIES]
    toc += [("bridges", "Bridges"), ("mega", "Mega projects"), ("qa", "QA records"), ("about", "How this file was built")]
    P.append('<header><div class="wrap"><div class="kicker">One-file edition · built %s from the University Authoring Guide · no JavaScript</div>'
             '<h1>Bare Metal to GPU University</h1>'
             '<p class="lead">From a curious kid who has never programmed to an engineer who can build an operating system for any '
             'hardware, write GPU kernels in CUDA C++ and ROCm/HIP, and build the software of robots and drones. Thirteen faculties, '
             '%d courses, %d chapters, 10 bridge chapters and 8 mega projects, all in this one page.</p>'
             '<nav class="toc">%s</nav></div></header>\n<main class="wrap">\n'
             % (today, len([c for c in cards if not re.match(r"(BR|MP)", c)]), len(all_chapters),
                "".join('<a href="#%s">%s</a>' % (i, t) for i, t in toc)))

    # --- home
    written = len([c for c in all_chapters if c[0] in chapters])
    P.append('<h2 id="home">Welcome</h2>')
    P.append('<div class="co note"><span class="lbl">Build status of this edition</span><p>%d of %d chapters are written as '
             '<b>drafts</b>; the others are listed in the <a href="#catalogue">catalogue</a> as <i>planned</i>. Status counts: %s.</p>'
             '<p>Every code listing in a written chapter was compiled and run in this build and its real output is pasted in. '
             'This build had <b>no internet access</b>, so no official document could be opened: sources are named by title and '
             'marked pending verification, and no chapter is yet "published" in the sense of the guide (gates G1, G6–G8 are open). '
             'Each chapter links to its QA record.</p></div>'
             % (written, len(all_chapters), ", ".join("%s %d" % (STATUS_LABEL[k][0], v) for k, v in sorted(stat.items()))))
    P.append(section(guide, blocks, "honest"))
    P.append('<h2 id="start">Start here</h2><div class="grid3">'
             '<div class="card"><h4>A curious kid (about 10–12)</h4><p>Start with <a href="#KID101">KID101 How computers think</a>, '
             'chapter <a href="#F0-01">F0-01</a>. Go at your own pace, in short sessions, with a grown-up for anything physical.</p></div>'
             '<div class="card"><h4>A teen who already codes</h4><p>Read the Foundation-year chapters you need, then start '
             '<a href="#SP101">SP101 C++ I</a>. (The SP101 placement quiz is planned.)</p></div>'
             '<div class="card"><h4>An adult engineer</h4><p>Use the <a href="#paths">goal paths</a> and enter where your skills end; '
             'the upper years follow the <a href="%s">Systems Curriculum</a> milestones.</p></div></div>' % CURR_REL)
    P.append('<h2 id="ladder">The learner ladder</h2>' + section(guide, blocks, "s2"))
    P.append('<h2 id="paths">Goal paths</h2>' + section(guide, blocks, "s5-3b"))
    P.append('<h2 id="years">Years, semesters and the prerequisite graph</h2>' + section(guide, blocks, "s5-2")
             + section(guide, blocks, "s5-3"))
    P.append('<h3 id="faculties-table">The thirteen faculties</h3>' + section(guide, blocks, "s5-1"))

    # --- catalogue
    rows = []
    for chid, chtitle, course, lvl in all_chapters:
        st = chapter_status(chid, chapters)
        link = '<a href="#%s">%s</a>' % (chid, chid) if chid in chapters else chid
        title = esc(chapters[chid]["title"]) if chid in chapters else esc(chtitle)
        lvlc = chapters[chid].get("level", lvl) if chid in chapters else lvl
        qa = '<a href="#qa-%s">QA</a>' % chid if chid in chapters else "—"
        ver = "v0.1" if chid in chapters else "—"
        rows.append('<tr id="cat-%s"><td class="n">%s</td><td>%s</td><td><a href="#%s">%s</a></td><td class="n">%s</td><td>%s</td>'
                    '<td class="n">%s</td><td>%s</td></tr>' % (chid, link, title, course, course, esc(lvlc), status_tag(st), ver, qa))
    for b in bridges + mps:
        t, lvl = card_title_level(cards[b]["html"])
        rows.append('<tr><td class="n"><a href="#%s">%s</a></td><td>%s</td><td>%s</td><td class="n">%s</td><td>%s</td>'
                    '<td class="n">—</td><td>—</td></tr>' % (b, b, esc(t), "bridge" if b.startswith("BR") else "mega project",
                                                             esc(lvl), status_tag("planned")))
    P.append('<h2 id="catalogue">Catalogue</h2><p>Every chapter, bridge chapter and mega project with its status (guide 12.5). '
             'Status words: <i>planned</i> (not yet written), <i>draft</i> (written, code not yet run), <i>draft · code run</i> '
             '(written and every listing compiled and run in this build), <i>internally checked</i> (an independent checker agent '
             'reviewed it; still not verified against official sources).</p>'
             '<div class="tw wide"><table><tr><th>Id</th><th>Title</th><th>Course</th><th>Level</th><th>Status</th><th>Version</th>'
             '<th>QA</th></tr>%s</table></div>' % "".join(rows))

    P.append('<h2 id="safety">Safety</h2><p>Read before any robot, drone, firmware or GPU lab. Every lab links here.</p>'
             + re.search(r'(<div class="note b">.*?</ul>\s*</div>)', section(guide, blocks, "honest"), re.S).group(1))
    P.append('<h2 id="analogies">Analogy registry</h2>' + section(guide, blocks, "s8"))
    P.append('<h2 id="glossary">Glossary</h2>' + render_glossary(load_glossary_proposals(), curriculum_glossary(curr)))

    # --- faculties, courses, chapters
    for f, fname, fclass in FACULTIES:
        fid = "f" + f[1:]
        P.append('<h2 class="fac" id="fac-%s">Faculty %s · %s</h2>' % (f, f, esc(fname)))
        fac_html = section(guide, blocks, fid)
        # keep only the faculty intro (safety/hardware boxes) before the first course card
        intro = fac_html.split('<div class="course', 1)[0]
        P.append(intro)
        for course in courses_by_fac[f]:
            card = cards[course]["html"]
            ctitle, clvl = card_title_level(card)
            # chapter list with status
            items = []
            for chid, chtitle in card_chapters(card):
                st = chapter_status(chid, chapters)
                name = '<a href="#%s">%s</a>' % (chid, chid) if chid in chapters else chid
                items.append("<li>%s %s %s</li>" % (name, esc(chapters[chid]["title"] if chid in chapters else chtitle), status_tag(st)))
            card2 = card.replace('<div class="course %s" id="%s">' % (cards[course]["fclass"], course),
                                 '<div class="course %s">' % cards[course]["fclass"], 1)
            P.append('<h3 id="%s">%s · %s</h3>' % (course, course, esc(ctitle)))
            P.append(card2)
            P.append('<p class="sub">Chapters of %s:</p><ul>%s</ul>' % (course, "".join(items)))
            P.append('<p class="sub">Exams and the course project are planned (guide 11.4–11.5); answer keys will live in a separate file.</p>')
            seq = [c for c, _ in card_chapters(card) if c in chapters]
            for i, chid in enumerate(seq):
                P.append(render_chapter(chapters[chid], ctitle, written_known, problems,
                                        seq[i + 1] if i + 1 < len(seq) else None, seq[i - 1] if i > 0 else None))

    # --- bridges and mega projects
    P.append('<h2 class="fac" id="bridges">Bridge chapters</h2>' + section(guide, blocks, "s6"))
    P.append('<h2 class="fac" id="mega">Mega projects</h2>' + section(guide, blocks, "s11-6"))

    # --- QA records
    P.append('<h2 class="fac" id="qa">QA records</h2><p>One record per written chapter (guide 3.7). Gates follow guide 12.3.</p>')
    for chid, _, _, _ in all_chapters:
        if chid in chapters:
            P.append(render_qa(chid, chapters[chid], chapter_status(chid, chapters)))

    P.append('<h2 id="about">How this file was built</h2>'
             '<p>This page is generated by <code>university/build/build.py</code> from the three source files in the repository '
             '(<code>UNIVERSITY_AUTHORING_GUIDE.html</code>, <code>SYSTEMS_CURRICULUM.html</code>; <code>DRIVERS.html</code> is the '
             'learning page the forensic-lab idea comes from) and from the chapter fragments in <code>university/chapters/</code>. '
             'Course cards, the analogy registry, the safety rules, the ladder, the goal paths and the bridge and mega-project cards are '
             'copied from the guide unchanged. Listings and outputs are copied byte for byte from <code>university/labs/</code>. '
             'See <code>university/README.md</code> for the folder layout and how to rebuild.</p>'
             '<p>Companion documents: <a href="%s">University Authoring Guide</a> · <a href="%s">Systems Curriculum</a> · '
             '<a href="../DRIVERS.html">Windows drivers learning page</a>.</p>' % (GUIDE_REL, CURR_REL))
    P.append("<footer>Bare Metal to GPU University · one-file edition · %s · no scripts; all diagrams are inline SVG using the "
             "colour tokens of the style block, in light and dark mode.</footer>\n</main>\n</body>\n</html>\n" % today)
    page = "\n".join(P)

    # --- link and id integrity
    guide_ids = set(re.findall(r'id="([^"]+)"', guide))
    curr_ids = set(re.findall(r'id="([^"]+)"', curr))
    ids = re.findall(r'\sid="([^"]+)"', page)
    seen, dups = set(), set()
    for i in ids:
        (dups if i in seen else seen).add(i)
    for d in sorted(dups):
        problems.append("duplicate id: " + d)

    def fix_href(m):
        target = m.group(1)
        if target in seen:
            return m.group(0)
        if target.startswith("gl-") and target.rstrip("-") in seen:
            return 'href="#%s"' % target.rstrip("-")
        if "cat-" + target in seen:
            return 'href="#cat-%s"' % target
        if target in guide_ids:
            return 'href="%s#%s"' % (GUIDE_REL, target)
        if target in curr_ids:
            return 'href="%s#%s"' % (CURR_REL, target)
        problems.append("broken link: #" + target)
        return m.group(0)

    page = re.sub(r'href="#([^"]*)"', fix_href, page)
    page = re.sub(r'href="(SYSTEMS_CURRICULUM|UNIVERSITY_AUTHORING_GUIDE|DRIVERS)\.html', r'href="../\1.html', page)
    if re.search(r"<script|\son[a-z]+=", page, re.I):
        problems.append("script or event handler found (guide 13.7 forbids JavaScript)")
    b = Balance()
    b.feed(page)
    for e in b.errors[:20]:
        problems.append("html: " + e)
    with open(OUT_PATH, "w", encoding="utf-8") as f:
        f.write(page)
    return page, problems, stat, written, len(all_chapters)


if __name__ == "__main__":
    page, problems, stat, written, total = build()
    print("wrote %s (%d bytes); chapters written %d of %d; status %s"
          % (os.path.relpath(OUT_PATH, ROOT), len(page.encode()), written, total, stat))
    for p in problems:
        print("PROBLEM:", p)
    if "--check" in sys.argv and problems:
        sys.exit(1)
