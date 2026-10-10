#!/usr/bin/env python3
"""Collect the "decisions for the owner" sections of every NOTES.md into
university/OWNER_DECISIONS.md, in queue order (courses, bridges, mega projects)."""
import pathlib, re

ROOT = pathlib.Path(__file__).resolve().parents[2]
UNI = ROOT / "university"
queue = [c for l in (UNI / "build/QUEUE.txt").read_text().splitlines()
         if l.strip() and not l.startswith("#") for c in l.split()]
out = ["# Decisions for the owner", "",
       "Collected by `university/build/decisions.py` from each unit's `NOTES.md`. "
       "Each section is the author's list, copied as written; the full notes (unverified "
       "claims, runs, analogy proposals) stay in the unit's own `NOTES.md`.", ""]
for c in queue:
    p = UNI / "chapters" / c / "NOTES.md"
    if not p.exists():
        continue
    text = p.read_text()
    secs = re.split(r"(?m)^(?=## )", text)
    picked = [s for s in secs if re.match(r"## .*(decision|owner)", s, re.I)]
    if not picked:
        picked = [s for s in secs if re.match(r"## ", s) and re.search(r"(?i)owner", s)]
    out.append(f"## {c}")
    out.append(f"Source: [`chapters/{c}/NOTES.md`](chapters/{c}/NOTES.md)")
    out.append("")
    if picked:
        for s in picked:
            out.append(re.sub(r"(?m)^(#+) ", lambda m: "#" + m.group(1) + " ", s.strip()))
            out.append("")
    else:
        out.append("No separate decision list; see the notes.")
        out.append("")
(UNI / "OWNER_DECISIONS.md").write_text("\n".join(out))
print(len(out), "lines")
