#!/usr/bin/env bash
# commit_course.sh COURSE "description" — commit and push one finished course
# (its chapter folder plus the lab folder of each chapter), then refresh PROGRESS.md.
set -e
cd "$(git -C "$(dirname "$0")" rev-parse --show-toplevel)"
c="$1"; d="$2"
paths="university/chapters/$c"
for f in university/chapters/$c/*.html; do id=$(basename "$f" .html)
  [ -d "university/labs/$id" ] && paths="$paths university/labs/$id"
  [ -f "university/_dossiers/$id.dossier.html" ] && paths="$paths university/_dossiers/$id.dossier.html"
  [ -f "university/qa/$id.json" ] && paths="$paths university/qa/$id.json"
done
[ -f "university/_keys/$c.keys.html" ] && paths="$paths university/_keys/$c.keys.html"
[ -d "university/labs/$c-P" ] && paths="$paths university/labs/$c-P"
git checkout -- university/UNIVERSITY.html 2>/dev/null || true
git add $paths
python3 university/build/progress.py --staged >/dev/null
git add university/build/PROGRESS.md
git commit -q -m "$c: $d

Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>
Claude-Session: https://claude.ai/code/session_01YTaPifkXsSrKf2pfSLfXHs"
for i in 1 2 3 4; do git push -q -u origin HEAD 2>&1 && break || sleep $((2**i)); done
git log --oneline -1
