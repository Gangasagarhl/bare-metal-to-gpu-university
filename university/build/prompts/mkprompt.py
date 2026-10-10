import sys,re
c=sys.argv[1]; hint=sys.argv[2] if len(sys.argv)>2 else ""
card=open('/tmp/claude-0/cards/%s.txt'%c).read()
ids=re.findall(r'(F\d+-\d+[a-z]?)\s',card.split('Chapters:')[1].split('Labs:')[0])
print(f"""You are the Author and Lab Engineer for one course of a "university" built as HTML, in the git repo at /home/claude/bare-metal-to-gpu-university (work only in that checkout; do not commit, push, or call any mcp__hearthbot__ tool; do not touch files outside your course's folders).

Read first, in full: university/build/AUTHOR_BRIEF.md, then university/build/AUTHOR_BRIEF_UPPER.md (it wins where they differ), then university/chapters/FRAGMENT_FORMAT.md. Then read the guide sections they name in UNIVERSITY_AUTHORING_GUIDE.html (large file: extract text with a small python script that strips tags; read its <style> block for the CSS classes), your faculty's section in guide 5 (safety/hardware boxes), the related bridge chapters in guide 6.2, and the curriculum milestones your card maps to in SYSTEMS_CURRICULUM.html. Look at one or two already-written chapters under university/chapters/ to match format and tone.

Your course card (copied from guide section 5):
{card}

Chapter ids to write: {', '.join(ids)} — one file each at university/chapters/{c}/<ID>.html, with labs under university/labs/<ID>/ (run each with `university/labs/run_lab.sh university/labs/<ID>` from the repo root and make sure it passes), plus university/chapters/{c}/glossary.json and university/chapters/{c}/NOTES.md.
{hint}
Every chapter has all 21 template sections plus the answers section, at least one inline SVG figure following the diagram rules (required types of guide 9.2 where they apply), claim tags, a Transition box, real code runs where code appears, and honest unverified / untested-on-hardware boxes where the build container cannot verify something. Validate each fragment (python html.parser balance check, ids prefixed with the chapter id, no URLs, no <script>). Finally run `python3 university/build/build.py` and fix any PROBLEM lines that mention your chapters (other courses' problems are not yours). Reply with a short summary: files written, listings run (pass / expected-fail / untested on hardware), unverified boxes, decisions for the owner.""")
