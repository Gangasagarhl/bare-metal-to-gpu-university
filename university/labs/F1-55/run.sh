#!/usr/bin/env bash
# F1-55 extra lab step: the forensic evidence. The SASS (sm_80) of the integrate kernel.
set -u
cd "$(dirname "$0")"
NVCC="$(nvcc --version | tail -n 2 | head -n 1)"
CMD="nvcc -arch=sm_80 -cubin stepper.cu -o .tmp.cubin && cuobjdump -sass .tmp.cubin | grep -E '^[[:space:]]+/\*[0-9a-f]{4}\*/' | sed -E 's#^[[:space:]]+/\*([0-9a-f]+)\*/[[:space:]]+(.*[^[:space:]])[[:space:]]*;.*#\1  \2#' | grep -v '  NOP\$'"
{
    echo "listing:   stepper.cu"
    echo "toolchain: $NVCC"
    echo "command:   $CMD"
    echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
    echo "hardware:  compiled only: no GPU is needed to read generated code"
} > stepper_sass.log
bash -c "$CMD" > stepper_sass.out 2>&1
rc=$?
echo "exit code: $rc" >> stepper_sass.log
rm -f .tmp.cubin
exit $rc
