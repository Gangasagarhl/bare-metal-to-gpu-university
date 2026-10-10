#!/usr/bin/env python3
"""Verification-pass queue. Usage: vqueue.py [verify|exam] [N]  -> next N pending units.
With no args prints a status table. A unit is verified when every chapter has
university/qa/<CH>.json with "factcheck": "done"; a course's exams are done when
chapters/<C>/EXAMS.html and _keys/<C>.keys.html both exist (bridges/MPs have no exams)."""
import json, pathlib, re, sys
ROOT = pathlib.Path(__file__).resolve().parents[3]; U = ROOT / "university"
units = [c for l in (U / "build/QUEUE.txt").read_text().splitlines()
         if l.strip() and not l.startswith("#") for c in l.split()]
def chapters(u):
    return sorted(p.stem for p in (U / "chapters" / u).glob("*.html")
                  if re.match(r"(F\d+-\d+[a-z]?|BR-\d+|MP\d+)$", p.stem))
def verified(u):
    for ch in chapters(u):
        q = U / "qa" / (ch + ".json")
        try:
            if json.loads(q.read_text()).get("factcheck") != "done": return False
        except Exception: return False
    return True
def is_course(u): return not re.match(r"(BR-|MP)", u)
def examed(u):
    return (not is_course(u)) or ((U / "chapters" / u / "EXAMS.html").exists() and (U / "_keys" / (u + ".keys.html")).exists())
if len(sys.argv) > 1:
    n = int(sys.argv[2]) if len(sys.argv) > 2 else 999
    if sys.argv[1] == "verify": print(" ".join([u for u in units if not verified(u)][:n]))
    else: print(" ".join([u for u in units if is_course(u) and verified(u) and not examed(u)][:n]))
    if sys.argv[1] == "chapters": pass
    sys.exit()
v = sum(verified(u) for u in units); e = sum(examed(u) for u in units if is_course(u))
print(f"verified {v}/{len(units)} units; exams {e}/{sum(is_course(u) for u in units)} courses")
