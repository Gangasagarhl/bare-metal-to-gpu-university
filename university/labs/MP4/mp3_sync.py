# MP4 Listing 14: fail when MP4's copy of the MP3 shape matrix drifts from MP3's own.
#   python3 -I mp3_sync.py ../MP3/mp3_suite.cpp shapes.hpp
import re
import sys


def triples(text):
    return [tuple(map(int, t)) for t in re.findall(r"\{\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\}", text)]


mp3 = open(sys.argv[1]).read()
# mp3_suite.cpp may define kShapes more than once under #if (a reduced set for a demonstration);
# the full matrix is the longest definition.
blocks = re.findall(r"kShapes\[\]\s*=\s*\{(.*?)\};", mp3, re.S)
theirs = max((triples(b) for b in blocks), key=len)
mine = triples(open(sys.argv[2]).read())[: len(theirs)]
print("MP3 shapes (%s): %s" % (sys.argv[1], theirs))
print("MP4 rows 1-%d (%s): %s" % (len(theirs), sys.argv[2], mine))
print("in sync" if mine == theirs else "DRIFT: update shapes.hpp and rerun the suite")
sys.exit(0 if mine == theirs else 1)
