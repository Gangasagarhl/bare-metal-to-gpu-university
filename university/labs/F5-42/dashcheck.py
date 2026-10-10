# dashcheck.py - checks the static HTML SLO report written by dashboard.cpp (F5-42).
# Uses only the Python standard library: html.parser walks the document and reports
# unbalanced tags, any script or event-handler attribute, and counts the chart bars and rows.
import sys
from html.parser import HTMLParser

VOID = {"meta", "br", "img", "input", "link", "hr", "line", "col"}


class Check(HTMLParser):
    def __init__(self):
        super().__init__()
        self.stack = []
        self.problems = []
        self.count = {}

    def handle_starttag(self, tag, attrs):
        self.count[tag] = self.count.get(tag, 0) + 1
        if tag == "script":
            self.problems.append("script element found")
        for name, _ in attrs:
            if name.startswith("on"):
                self.problems.append("event handler attribute: " + name)
        if tag not in VOID:
            self.stack.append(tag)

    def handle_startendtag(self, tag, attrs):
        self.count[tag] = self.count.get(tag, 0) + 1

    def handle_endtag(self, tag):
        if tag in VOID:
            return
        if not self.stack or self.stack[-1] != tag:
            self.problems.append("unbalanced </%s> at line %d" % (tag, self.getpos()[0]))
        else:
            self.stack.pop()


text = open(sys.argv[1], encoding="utf-8").read()
c = Check()
c.feed(text)
c.close()
if c.stack:
    c.problems.append("unclosed: " + ", ".join(c.stack))
print("bytes: %d" % len(text.encode("utf-8")))
print("bars (rect): %d   table rows (tr): %d   svg: %d" % (c.count.get("rect", 0), c.count.get("tr", 0), c.count.get("svg", 0)))
print("problems: %d" % len(c.problems))
for p in c.problems:
    print("  " + p)
ok = not c.problems and c.count.get("rect", 0) == 30 and c.count.get("tr", 0) == 31
print("result: %s" % ("PASS" if ok else "FAIL"))
sys.exit(0 if ok else 1)
