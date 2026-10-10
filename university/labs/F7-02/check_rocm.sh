#!/usr/bin/env bash
# check_rocm.sh - a ROCm readiness checklist for one Linux machine (F7-02).
# It only reads; it changes nothing. Every line says PASS, FAIL or INFO.
# The list of supported GPU targets is NOT built in: copy it from the ROCm
# compatibility matrix for your ROCm version into supported_targets.txt.
set -u
fails=0
pass() { echo "PASS  $1"; }
fail() { echo "FAIL  $1"; fails=$((fails + 1)); }
info() { echo "INFO  $1"; }

# 1. The toolchain answers
if command -v hipcc > /dev/null; then
    pass "hipcc found: $(hipcc --version 2>/dev/null | head -n 1)"
else
    fail "hipcc not found on PATH"
fi
info "hipconfig --platform: $(hipconfig --platform 2>/dev/null || echo '?')"

# 2. The kernel-side interfaces the runtime opens (see the unverified box in F7-02)
if [ -e /dev/kfd ]; then pass "/dev/kfd exists"; else fail "/dev/kfd missing"; fi
if ls /dev/dri/renderD* > /dev/null 2>&1; then
    pass "render nodes: $(ls /dev/dri/renderD* | tr '\n' ' ')"
else
    fail "no /dev/dri/renderD* render nodes"
fi
info "this user's groups: $(id -nG)"

# 3. The runtime's own view of the machine
agents="$(rocminfo 2>&1 | sed 's/\x1b\[[0-9;]*m//g')"
gpus="$(printf '%s\n' "$agents" | grep -oE 'gfx[0-9a-f]+' | sort -u | tr '\n' ' ')"
if [ -n "$gpus" ]; then
    pass "rocminfo lists GPU agents: $gpus"
else
    fail "rocminfo lists no GPU agent; its first line: $(printf '%s\n' "$agents" | head -n 1)"
fi
info "rocm_agent_enumerator: $(rocm_agent_enumerator 2>&1 | tr '\n' ' ')"

# 4. Is each GPU target on the list you copied from the compatibility matrix?
list="$(dirname "$0")/supported_targets.txt"
if [ ! -s "$list" ] || ! grep -qE '^gfx' "$list"; then
    info "supported_targets.txt has no targets yet: copy them from the ROCm compatibility matrix"
else
    for g in $gpus; do
        if grep -qx "$g" "$list"; then pass "$g is on your supported list"; else fail "$g is not on your supported list"; fi
    done
fi

echo "checks failed: $fails"
[ "$fails" -eq 0 ]
