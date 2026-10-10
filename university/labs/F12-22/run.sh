#!/usr/bin/env bash
# F12-22 run.sh: the seam edit as a real diff (step seam_diff). The C++ listings are built and
# run by run_lab.sh itself.
set -u
cd "$(dirname "$0")"
{
    echo "listing:   legacy_status.hpp, legacy_seam.hpp (run by run.sh)"
    echo "toolchain: $(diff --version | head -n 1)"
    echo "command:   diff -u --label legacy_status.hpp --label legacy_seam.hpp legacy_status.hpp legacy_seam.hpp"
    echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
} > seam_diff.log
diff -u --label legacy_status.hpp --label legacy_seam.hpp legacy_status.hpp legacy_seam.hpp \
    > seam_diff.out
rc=$?
echo "exit code: $rc (diff returns 1 when the files differ, as expected here)" >> seam_diff.log
[ "$rc" = 1 ] && exit 0
exit 1
