#!/usr/bin/env bash
# Prints g++'s own one-line description of the warning flags this course uses.
# Run from this folder:  bash help_flags.sh > help_flags.txt 2>&1
# The awk filter keeps each matching option line and its indented continuation lines.
set -u
pick='/^  -(Wall|Wextra|Wpedantic|Werror|Wunused-variable|Wparentheses|Wuninitialized|Wreturn-type) /{p=1; print; next} /^  -/{p=0} p && /^      /{print}'
echo '$ g++ --version | head -n 1'
g++ --version | head -n 1
echo '$ g++ -v --help   (filtered to the flags used in this course)'
g++ -v --help 2>/dev/null | awk "$pick"
