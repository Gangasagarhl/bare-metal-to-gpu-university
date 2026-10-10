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
    "checked": ("code run · internally checked", "tok"),
    "checked-hw": ("code run · internally checked · hardware steps untested", "tok"),
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
nav.chtoc{background:var(--paper);border:1px solid var(--line);border-radius:10px;padding:8px 14px;margin:10px 0 16px;font-size:13.5px;line-height:1.9}
nav.chtoc b{margin-right:6px}
nav.chtoc a{margin-right:10px;white-space:nowrap}
details.labfiles,details.tree{border:1px solid var(--line);border-radius:10px;padding:6px 12px;margin:14px 0;background:var(--paper)}
details.labfiles>summary,details.tree>summary{cursor:pointer;font-weight:700}
details.labfiles h4{margin:14px 0 4px;font-family:var(--mono,monospace);font-size:13px}
details.tree details.tree{margin:6px 0 6px 10px}
ul.tree{margin:4px 0 8px 18px;padding:0}
ul.tree li{margin:2px 0}
ul.tree ul{font-size:13px;margin:2px 0 6px 16px}
section.exams,div.keys,div.dossier{border-top:4px solid var(--line);margin-top:40px;padding-top:6px}
"""


# ----------------------------------------------------------------------------- helpers

def read(path):
    with open(path, encoding="utf-8") as f:
        return f.read()


def esc(s):
    return html.escape(s, quote=False)


def slug(term):
    return "gl-" + re.sub(r"[^a-z0-9]+", "-", term.lower()).strip("-")


def md_to_html(text):
    """Tiny Markdown subset (headings, bullets, bold, code, links, paragraphs) for the owner files."""
    out, para, lst = [], [], []

    def inline(t):
        t = esc(t)
        t = re.sub(r"\*\*(.+?)\*\*", r"<b>\1</b>", t)
        t = re.sub(r"`([^`]+)`", r"<code>\1</code>", t)
        t = re.sub(r"\[([^\]]+)\]\(([^)]+)\)", r"<code>\1</code>", t)
        return t

    def flush():
        if para:
            out.append("<p>%s</p>" % inline(" ".join(para)))
            para.clear()
        if lst:
            out.append("<ul>%s</ul>" % "".join("<li>%s</li>" % inline(x) for x in lst))
            lst.clear()

    for line in text.splitlines():
        m = re.match(r"(#{1,4})\s+(.*)", line)
        if m:
            flush()
            lvl = min(len(m.group(1)) + 2, 5)
            out.append("<h%d>%s</h%d>" % (lvl, inline(m.group(2)), lvl))
        elif re.match(r"\s*[-*]\s+", line) and not line.startswith("  "):
            if para:
                flush()
            lst.append(re.sub(r"^\s*[-*]\s+", "", line))
        elif line.strip() == "":
            flush()
        elif lst:
            lst[-1] += " " + line.strip()
        else:
            para.append(line.strip())
    flush()
    return "\n".join(out)


def load_extra(path):
    """An HTML fragment with an optional meta comment; returns its body or None."""
    if not os.path.exists(path):
        return None
    src = read(path)
    m = re.match(r"\s*<!--meta.*?-->", src, re.S)
    return src[m.end():] if m else src


def chapter_toc(cid, body, extra):
    items = re.findall(r'<h2[^>]*\sid="(%s-[^"]+)"[^>]*>(.*?)</h2>' % re.escape(cid), body, re.S)
    links = []
    for i, t in items:
        links.append('<a href="#%s">%s</a>' % (i, re.sub(r"<[^>]+>", "", t).strip()))
        if i.endswith("-able") and ('id="%s-jargon"' % cid) in body:
            links.append('<a href="#%s-jargon">Jargon box</a>' % cid)
        if i.endswith("-mistakes") and ('id="%s-transition"' % cid) in body:
            links.append('<a href="#%s-transition">Transition box</a>' % cid)
    links += extra
    return '<nav class="chtoc" id="%s-toc"><b>In this chapter:</b>%s</nav>' % (cid, " ".join(links))


TEXT_LIMIT = 300_000


def render_lab_files(cid, shown=()):
    """Every file of a lab folder; files already printed in the chapter are referenced, not repeated."""
    shown = set(shown)
    d = os.path.join(UNI, "labs", cid)
    if not os.path.isdir(d):
        return ""
    files = []
    for root, dirs, fns in os.walk(d):
        dirs.sort()
        for fn in sorted(fns):
            files.append(os.path.relpath(os.path.join(root, fn), d))
    parts = []
    for rel in files:
        p = os.path.join(d, rel)
        size = os.path.getsize(p)
        if "%s/%s" % (cid, rel) in shown:
            parts.append("<h4>%s</h4><p class=\"sub\">Shown in full in the chapter above.</p>" % esc(rel))
            continue
        try:
            raw = open(p, "rb").read()
            if b"\0" in raw[:4096]:
                raise UnicodeDecodeError("bin", b"", 0, 1, "binary")
            txt = raw.decode("utf-8")
        except UnicodeDecodeError:
            parts.append("<h4>%s</h4><p class=\"sub\">Binary file (%d bytes), kept in the repository; not shown.</p>" % (esc(rel), size))
            continue
        if size > TEXT_LIMIT:
            parts.append("<h4>%s</h4><p class=\"sub\">Text file of %d bytes, too large to show here; it is in "
                         "<code>university/labs/%s/%s</code>.</p>" % (esc(rel), size, esc(cid), esc(rel)))
            continue
        parts.append("<h4>%s</h4><pre><code>%s</code></pre>" % (esc(rel), html.escape(txt, quote=True) if txt.strip() else "(empty file)"))
    return ('<details class="labfiles" id="%s-labfiles"><summary>Lab files of %s: every source, input, real output and '
            'run record (%d files, from <code>university/labs/%s/</code>)</summary>%s</details>'
            % (cid, cid, len(files), esc(cid), "".join(parts)))


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
        return "checked-hw" if qa.get("hardware_untested") else "checked"
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
    group = {"BR": "bridges", "MP": "mega"}.get(meta["course"])
    if group:
        nav.append('<a href="#%s">%s</a>' % (group, esc(course_title.lower())))
    else:
        nav.append('<a href="#%s">course %s</a>' % (meta["course"], meta["course"]))
    nav.append('<a href="#catalogue">catalogue</a>')
    if nxt:
        nav.append('<a href="#%s">%s →</a>' % (nxt, nxt))
    front = [
        ("Course", '<a href="#%s">%s</a>' % (group, esc(course_title)) if group else
                   '<a href="#%s">%s %s</a>' % (meta["course"], meta["course"], esc(course_title))),
        ("Chapter id", "%s %s" % (cid, esc(meta["title"]))),
        ("Level", lvl),
        ("Prerequisites", linkify_ids(esc(meta.get("prereqs", "none")), known)),
        ("Time to study", esc(meta.get("time", ""))),
        ("Hardware needed", esc(meta.get("hardware", ""))),
        ("Sources dossier", ('<a href="#dossier-%s">Source dossier of %s</a> (in this file): the documents opened, the facts used '
                             'and what could not be confirmed.' % (cid, cid))
         if os.path.exists(os.path.join(UNI, "_dossiers", cid + ".dossier.html")) else
         'Not yet written: sources are named by title in the chapter\'s Sources section and marked "pending verification" (gate G1 open).'),
        ("QA record", '<a href="#qa-%s">QA record of %s</a> (in this file)' % (cid, cid)),
        ("Version", ("v0.2 · %s · verified against the sources in its dossier" % status_tag(st)) if st.startswith("checked")
         else "v0.1 draft · %s · not yet verified against sources" % status_tag(st)),
        ("Maps to", linkify_ids(esc(meta.get("maps", "")), known)),
    ]
    fm = "".join("<dt>%s</dt><dd>%s</dd>" % (k, v) for k, v in front)
    extra = []
    shown = set(re.findall(r'data-src="([^"]+)"', meta["body"]))
    for r in re.findall(r'data-run="([^"]+)"', meta["body"]):
        shown.add(r + ".out")
    labs_html = render_lab_files(cid, shown)
    if labs_html:
        extra.append('<a href="#%s-labfiles">Lab files</a>' % cid)
    if os.path.exists(os.path.join(UNI, "_dossiers", cid + ".dossier.html")):
        extra.append('<a href="#dossier-%s">Source dossier</a>' % cid)
    extra.append('<a href="#qa-%s">QA record</a>' % cid)
    body = chapter_toc(cid, body, extra) + "\n" + body + "\n" + labs_html
    return (
        '<article class="chapter" id="%s">\n<div class="kicker">%s · Chapter %s</div>\n'
        '<h2 class="chtitle">%s</h2>\n<p class="back">%s</p>\n<dl class="front">%s</dl>\n%s\n'
        '<footer>Corrections log: <a href="#qa-%s-corrections">%s corrections</a> (%d recorded). %s</footer>\n</article>\n'
        % (cid, esc(meta["course"] + " · " + course_title), cid, esc(meta["title"]), " · ".join(nav), fm, body, cid, cid,
           len((qa or {}).get("corrections") or []), " · ".join(nav))
    )


def render_qa(cid, meta, st):
    runs = lab_runs(cid)
    qa = load_qa(cid) or {}
    rows = [
        ("G0", "Planned (course card)", "Dean (this build)", "ok — scope copied from the guide's course card"),
        ("G1", "Dossier approved", qa.get("checker", "—") if qa.get("dossier") else "—",
         esc(qa.get("dossier", "open — no official source opened yet"))),
        ("G2", "Outline approved", "—", "skipped in batch 1 (Author wrote directly to the template)"),
        ("G3", "Draft complete", "Author agent", "ok — all 21 template sections present" if meta else "—"),
        ("G4", "Diagrams reviewed", "Diagram reviewer agent" if qa.get("diagrams") else "—", esc(qa.get("diagrams", "open"))),
        ("G5", "Labs and code tested", "Lab Engineer agent",
         ("ok — %d listing run(s), records below" % len(runs)) if runs and all(run_ok(l) for _, l in runs)
         else ("no listings" if not runs else "FAILED — see records")),
        ("G6", "Fact-check passed", esc(qa.get("checker", "—")), esc(qa.get("factcheck_note", "open"))),
        ("G7", "Edit + reading level", "Editor agent" if qa.get("edit") else "—", esc(qa.get("edit", "open"))),
        ("G8", "Accessibility", "Accessibility reviewer agent" if qa.get("accessibility") else "—", esc(qa.get("accessibility", "open"))),
        ("G9", "Integrated", "Integrator (build.py)", "ok — assembled into this file; links and ids checked"),
    ]
    t = "".join("<tr><td>%s</td><td>%s</td><td>%s</td><td>%s</td></tr>" % r for r in rows)
    runs_html = ""
    if runs:
        runs_html = "<h4>Code runs</h4><div class=\"tw\"><table><tr><th>Listing</th><th>Record</th></tr>%s</table></div>" % "".join(
            "<tr><td><code>labs/%s/%s</code></td><td><pre>%s</pre></td></tr>" % (cid, esc(n), esc(l.strip())) for n, l in runs)
    findings = ""
    if qa.get("findings"):
        findings = "<h4>Fact-check findings</h4><ul>%s</ul>" % "".join("<li>%s</li>" % esc(str(f)) for f in qa["findings"])
    if qa.get("hardware_untested"):
        findings += ("<h4>Untested on hardware (approved by the owner, ruling C3)</h4><ul>%s</ul>"
                     % "".join("<li>%s</li>" % esc(str(f)) for f in qa["hardware_untested"]))
    corr = qa.get("corrections") or []
    if corr:
        rows_c = "".join("<tr><td>%s</td><td>%s</td><td>%s</td><td>%s</td></tr>" % tuple(
            esc(str(c.get(k, ""))) if isinstance(c, dict) else (esc(str(c)) if k == "now" else "")
            for k in ("where", "was", "now", "source")) for c in corr)
        corr_html = ('<div class="tw"><table><tr><th>Where</th><th>Was</th><th>Now</th><th>Source</th></tr>%s</table></div>' % rows_c)
    else:
        corr_html = "no corrections recorded."
    return ('<div class="card" id="qa-%s"><h4>QA record — %s %s %s</h4><div class="tw"><table><tr><th>Gate</th><th>What</th>'
            '<th>By</th><th>Result</th></tr>%s</table></div>%s%s<div id="qa-%s-corrections"><b>Corrections log:</b> %s</div>'
            '<p><a href="#%s">back to the chapter</a></p></div>\n'
            % (cid, cid, esc(meta.get("title", "")), status_tag(st), t, runs_html, findings, cid, corr_html, cid))


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


def render_contents(cards, courses_by_fac, chapters, bridges, mps):
    """Full contents tree: faculty > course > chapter > sections, labs, Q&A (no JavaScript: <details>)."""
    def chapter_items(chid):
        if chid not in chapters:
            return ""
        body = chapters[chid]["body"]
        secs = re.findall(r'<h2[^>]*\sid="(%s-[^"]+)"[^>]*>(.*?)</h2>' % re.escape(chid), body, re.S)
        links = ['<a href="#%s">%s</a>' % (i, re.sub(r"<[^>]+>", "", t).strip()) for i, t in secs]
        if os.path.isdir(os.path.join(UNI, "labs", chid)):
            links.append('<a href="#%s-labfiles">lab files</a>' % chid)
        return "<ul><li>%s</li></ul>" % " · ".join(links)

    def ch_line(chid, title):
        t = esc(chapters[chid]["title"]) if chid in chapters else esc(title)
        return '<li><a href="#%s">%s</a> %s%s</li>' % (chid, chid, t, chapter_items(chid))

    out = ['<h2 id="contents">Contents</h2><p>Every faculty, course, chapter and chapter section, with the labs and the '
           'questions and answers (each chapter\'s "Check yourself" and "Answers to Check yourself"). Open a branch to see '
           'what is inside.</p>']
    for f, fname, _ in FACULTIES:
        courses = courses_by_fac[f]
        inner = []
        for c in courses:
            ctitle, _ = card_title_level(cards[c]["html"])
            chs = "".join(ch_line(chid, t) for chid, t in card_chapters(cards[c]["html"]))
            ex = ('<li><a href="#%s-exams">%s exams and course project</a> · <a href="#keys-%s">answer key</a></li>' % (c, c, c)
                  if os.path.exists(os.path.join(UNI, "chapters", c, "EXAMS.html")) else "")
            inner.append('<details class="tree"><summary><a href="#%s">%s</a> %s</summary><ul class="tree">%s%s</ul></details>'
                         % (c, c, esc(ctitle), chs, ex))
        out.append('<details class="tree"><summary><a href="#fac-%s">Faculty %s</a> · %s (%d courses)</summary>%s</details>'
                   % (f, f, esc(fname), len(courses), "".join(inner)))
    for gid, title, group in (("bridges", "Bridge chapters", bridges), ("mega", "Mega projects", mps)):
        items = "".join(ch_line(b, card_title_level(cards[b]["html"])[0]) for b in group)
        out.append('<details class="tree"><summary><a href="#%s">%s</a> (%d)</summary><ul class="tree">%s</ul></details>'
                   % (gid, title, len(group), items))
    out.append('<p><a href="#keys">Answer keys</a> · <a href="#dossiers">Source dossiers</a> · <a href="#qa">QA records</a> · '
               '<a href="#glossary">Glossary</a></p>')
    return "\n".join(out)


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
        if cid not in {c[0] for c in all_chapters} and cid not in bridges and cid not in mps:
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
    toc = [("home", "Home"), ("contents", "Contents"), ("start", "Start here"), ("ladder", "Levels"), ("paths", "Goal paths"), ("years", "Years"),
           ("catalogue", "Catalogue"), ("safety", "Safety"), ("kit", "Kit"), ("rulings", "Owner rulings"), ("analogies", "Analogies"),
           ("glossary", "Glossary")]
    toc += [("fac-" + f, f) for f, _, _ in FACULTIES]
    toc += [("bridges", "Bridges"), ("mega", "Mega projects"), ("keys", "Answer keys"), ("dossiers", "Source dossiers"),
            ("qa", "QA records"), ("about", "How this file was built")]
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
    n_checked = stat.get("checked", 0) + stat.get("checked-hw", 0)
    P.append('<div class="co note"><span class="lbl">Build status of this edition</span><p>%d of %d chapters are written, plus '
             'the 10 bridge chapters and the 8 mega-project handbooks. %d of the %d chapters are verified against their sources '
             '(each has a <a href="#dossiers">source dossier</a> and a QA record). Status counts: %s.</p>'
             '<p>Every code listing was compiled and run in the build container and its real output is pasted in. The container '
             'has no GPU, boards, robot or drone: GPU code was compiled with the real compilers, firmware ran in QEMU and robots '
             'and drones in simulators, and every such step is marked <i>untested on hardware</i>. The owner\'s decisions are in '
             '<a href="#rulings">Owner rulings</a> and the chosen hardware in <a href="#kit">Reference kit</a>.</p></div>'
             % (written, len(all_chapters), n_checked, len(all_chapters),
                ", ".join("%s %d" % (STATUS_LABEL[k][0], v) for k, v in sorted(stat.items()))))
    P.append(section(guide, blocks, "honest"))
    P.append(render_contents(cards, courses_by_fac, chapters, bridges, mps))
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
        st = chapter_status(b, chapters)
        if b in chapters:
            t = chapters[b]["title"]
            lvl = chapters[b].get("level", lvl)
        rows.append('<tr id="cat-%s"><td class="n"><a href="#%s">%s</a></td><td>%s</td><td>%s</td><td class="n">%s</td><td>%s</td>'
                    '<td class="n">%s</td><td>%s</td></tr>'
                    % (b, b, b, esc(t), '<a href="#bridges">bridge</a>' if b.startswith("BR") else '<a href="#mega">mega project</a>',
                       esc(lvl), status_tag(st), "v0.1" if b in chapters else "—",
                       '<a href="#qa-%s">QA</a>' % b if b in chapters else "—"))
    P.append('<h2 id="catalogue">Catalogue</h2><p>Every chapter, bridge chapter and mega project with its status (guide 12.5). '
             'Status words: <i>planned</i> (not yet written), <i>draft</i> (written, code not yet run), <i>draft · code run</i> '
             '(written and every listing compiled and run in this build), <i>internally checked</i> (an independent checker agent '
             'reviewed it; still not verified against official sources).</p>'
             '<div class="tw wide"><table><tr><th>Id</th><th>Title</th><th>Course</th><th>Level</th><th>Status</th><th>Version</th>'
             '<th>QA</th></tr>%s</table></div>' % "".join(rows))

    P.append('<h2 id="safety">Safety</h2><p>Read before any robot, drone, firmware or GPU lab. Every lab links here.</p>'
             + re.search(r'(<div class="note b">.*?</ul>\s*</div>)', section(guide, blocks, "honest"), re.S).group(1))
    kit = load_extra(os.path.join(UNI, "build", "KIT.fragment.html"))
    P.append('<h2 id="kit">Reference kit</h2><p>The hardware and software versions the labs refer to, chosen by the owner '
             '(ruling D1) and checked against the vendors\' and projects\' own pages.</p>'
             + (kit if kit is not None else "<p>Not chosen yet.</p>"))
    rul = os.path.join(UNI, "OWNER_RULINGS.md")
    P.append('<div id="rulings">%s</div>' % (md_to_html(read(rul)) if os.path.exists(rul) else "<p>No rulings yet.</p>"))
    P.append('<h2 id="analogies">Analogy registry</h2>' + section(guide, blocks, "s8"))
    adds = load_extra(os.path.join(UNI, "chapters", "ANALOGY_ADDITIONS.html"))
    if adds is not None:
        P.append('<h3 id="analogy-additions">Analogy registry additions (approved by the owner, ruling A3)</h3>' + adds)
    P.append('<h2 id="glossary">Glossary</h2>' + render_glossary(load_glossary_proposals(), curriculum_glossary(curr)))

    # --- faculties, courses, chapters
    exam_courses = []
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
            exams = load_extra(os.path.join(UNI, "chapters", course, "EXAMS.html"))
            if exams is not None:
                P.append('<p class="sub">Exams and the course project: <a href="#%s-exams">%s exams</a>, after the chapters; '
                         'answer keys in the <a href="#keys">Answer keys</a> appendix.</p>' % (course, course))
            else:
                P.append('<p class="sub">Exams and the course project are planned (guide 11.4–11.5).</p>')
            seq = [c for c, _ in card_chapters(card) if c in chapters]
            for i, chid in enumerate(seq):
                P.append(render_chapter(chapters[chid], ctitle, written_known, problems,
                                        seq[i + 1] if i + 1 < len(seq) else None, seq[i - 1] if i > 0 else None))
            if exams is not None:
                P.append('<section class="exams" id="%s-exams"><div class="kicker">%s · Exams and course project</div>'
                         '<h2 class="chtitle">%s exams and course project</h2>%s'
                         '<p class="back"><a href="#keys-%s">answer key</a> · <a href="#%s">course %s</a></p></section>'
                         % (course, course, course, exams, course, course, course))
                exam_courses.append(course)

    # --- bridges and mega projects
    for hid, sec, group, gtitle in (("bridges", "s6", bridges, "Bridge chapters"), ("mega", "s11-6", mps, "Mega projects")):
        html_ = section(guide, blocks, sec)
        for b in group:
            if b in chapters:  # the written chapter owns the anchor; the card stays as the summary
                html_ = html_.replace(' id="%s"' % b, '', 1)
        P.append('<h2 class="fac" id="%s">%s</h2>' % (hid, gtitle) + html_)
        seq = [b for b in group if b in chapters]
        for i, b in enumerate(seq):
            P.append(render_chapter(chapters[b], gtitle, written_known, problems,
                                    seq[i + 1] if i + 1 < len(seq) else None, seq[i - 1] if i > 0 else None))

    # --- appendices: answer keys and source dossiers
    keys = [(c, load_extra(os.path.join(UNI, "_keys", c + ".keys.html"))) for c in exam_courses]
    keys = [(c, k) for c, k in keys if k is not None]
    P.append('<h2 class="fac" id="keys">Answer keys</h2><p>Model answers and marking points for every course\'s midterm, final '
             'and practical (guide 11.4). Chapter quiz answers are at the end of each chapter ("Answers to Check yourself"). '
             'Learners: try the exam first.</p>')
    for c, k in keys:
        # practical folders hold reference solutions and hidden evidence (ruling B7): markers' material, so it sits with the key
        P.append('<div class="keys" id="keys-%s"><h3>%s answer key</h3>%s%s<p class="back"><a href="#%s-exams">back to the %s exams</a></p></div>'
                 % (c, c, k, render_lab_files(c + "-P"), c, c))
    if not keys:
        P.append("<p>No answer keys yet.</p>")
    P.append('<h2 class="fac" id="dossiers">Source dossiers</h2><p>One dossier per chapter (guide 3.2): every document the '
             'Source Researcher opened, its version, the sections used, the date and how it was accessed, the facts the chapter '
             'may use, and the facts that could not be confirmed. Web addresses are shown as plain text.</p>')
    n_dos = 0
    for chid in [c[0] for c in all_chapters] + bridges + mps:
        d = load_extra(os.path.join(UNI, "_dossiers", chid + ".dossier.html"))
        if d is not None and chid in chapters:
            n_dos += 1
            P.append('<div class="dossier" id="dossier-%s"><h3>Source dossier — %s %s</h3>%s<p class="back"><a href="#%s">back to '
                     'the chapter</a></p></div>' % (chid, chid, esc(chapters[chid]["title"]), d, chid))
    if not n_dos:
        P.append("<p>No dossiers yet.</p>")

    # --- QA records
    P.append('<h2 class="fac" id="qa">QA records</h2><p>One record per written chapter (guide 3.7). Gates follow guide 12.3.</p>')
    for chid in [c[0] for c in all_chapters] + bridges + mps:
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
