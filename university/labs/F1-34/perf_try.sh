#!/usr/bin/env bash
# Try the perf tool on the traversal program, exactly as a learner would on their own machine.
perf stat -e cache-misses,cache-references ./traversal 2048 cols
