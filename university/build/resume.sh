#!/usr/bin/env bash
# resume.sh — show build progress and the next courses to write after an interruption.
# Uncommitted course folders are partial work from an interrupted run: they are listed,
# not deleted, so the next author agent can finish them.
set -e
cd "$(git -C "$(dirname "$0")" rev-parse --show-toplevel)"
python3 university/build/progress.py
echo
echo "Partial (uncommitted) course folders from an interrupted run:"
git status --porcelain --untracked-files=normal university/chapters university/labs | sed 's/^/  /' || true
echo
echo "Next 10 courses to start:"
for c in $(python3 university/build/progress.py --next 10); do
  echo "  $c  -> university/build/prompts/$c.txt"
done
