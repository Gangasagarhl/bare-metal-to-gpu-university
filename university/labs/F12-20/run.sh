#!/usr/bin/env bash
# F12-20 run.sh: evidence for the forensic lab "Where does this log line come from?".
#   step server_log     : start Python's own http.server on loopback, make two requests that
#                         fail, keep the server's log lines (stderr) as the evidence pack.
#   step probe          : Listing 2 (probe.py) finds the caller of each send_error() for real.
#   step logline_search : the search session of the answer key, run for real with grep over
#                         the installed Python standard library.
set -u
cd "$(dirname "$0")"
status=0
PY=/usr/bin/python3
LIB="$($PY -c 'import os, http.server; print(os.path.dirname(os.path.dirname(http.server.__file__)))')"
head_log() {  # head_log <name> <listing> <toolchain line> <command>
    {
        echo "listing:   $2 (run by run.sh)"
        echo "toolchain: $3"
        echo "command:   $4"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
    } > "$1.log"
}

# ---- step 1: server_log ------------------------------------------------------------------
pyver="$($PY --version 2>&1)"
port="$($PY -c 'import socket; s=socket.socket(); s.bind(("127.0.0.1", 0)); print(s.getsockname()[1])')"
head_log server_log run.sh "$pyver" \
    "python3 -m http.server PORT --bind 127.0.0.1 (in an empty folder); GET /nope.txt; GET /missing-dir/"
www="$(mktemp -d)"
( cd "$www" && exec timeout 20 $PY -m http.server "$port" --bind 127.0.0.1 ) > /dev/null 2> server.stderr &
srv=$!
for _ in $(seq 1 50); do
    $PY -c "import socket; socket.create_connection(('127.0.0.1', $port), 0.2).close()" \
        2>/dev/null && break
    sleep 0.1
done
for path in /nope.txt /missing-dir/; do
    $PY -c "import http.client
c = http.client.HTTPConnection('127.0.0.1', $port, timeout=5)
c.request('GET', '$path')
print('client saw status', c.getresponse().status)" >> server.client
done
kill "$srv" 2>/dev/null; wait "$srv" 2>/dev/null
# keep only the per-request log lines; the start-up banner contains a URL and is dropped
grep -v '^Serving HTTP' server.stderr | grep -v '^Keyboard' | grep -v '^$' > server_log.out
n="$(wc -l < server_log.out)"
if [ "$n" = 4 ] && [ "$(grep -c 'status 404' server.client)" = 2 ]; then
    echo "exit code: 0 (4 log lines; the client saw status 404 twice)" >> server_log.log
else
    echo "exit code: 1 (expected 4 log lines and two 404s, got $n lines)" >> server_log.log
    status=1
fi
echo "note:      the start-up banner line was removed (it contains a URL); nothing else edited" \
    >> server_log.log
rm -rf "$www" server.stderr server.client

# ---- step 2: probe (Listing 2) -----------------------------------------------------------
head_log probe probe.py "$pyver" "python3 probe.py"
timeout 20 $PY probe.py > probe.out 2>&1; rc=$?
echo "exit code: $rc" >> probe.log
[ "$rc" = 0 ] || status=1

# ---- step 3: logline_search --------------------------------------------------------------
head_log logline_search run.sh "$(grep --version | head -n 1); $pyver standard library" \
    "grep -rnF --include=*.py <fragment> LIB  (LIB = the standard library folder of $pyver)"
{
    for frag in 'code 404, message File not found' 'code %d, message' '"File not found"' \
                ' - - [' 'def log_request' 'self.log_request(' 'do_GET' "'do_' +"; do
        hits="$(cd "$LIB" && grep -rnF --include='*.py' -- "$frag" . | sed 's#^\./##')"
        count=0; [ -n "$hits" ] && count="$(printf '%s\n' "$hits" | wc -l)"
        echo "\$ grep -rnF '$frag'   -> $count hit(s)"
        [ -n "$hits" ] && printf '%s\n' "$hits" | cut -c1-110
    done
} > logline_search.out
echo "exit code: 0" >> logline_search.log

# ---- step 4: hub_codecs: who imports the hub "codecs", by area? -----------------------------
head_log hub_codecs run.sh "$(grep --version | head -n 1); $pyver standard library" \
    "grep -rlE --include=*.py '^[[:space:]]*(import|from) codecs([ .,]|\$)' LIB, counted per area"
(cd "$LIB" && grep -rlE --include='*.py' '^[[:space:]]*(import|from) codecs([ .,]|$)' . \
    | sed 's#^\./##' | awk -F/ '{ a = (NF > 1) ? $1 "/" : "(top-level .py files)"; n[a]++; t++ }
        END { for (k in n) printf "%6d  %s\n", n[k], k; printf "%6d  total files importing codecs\n", t }' \
    | sort -rn) > hub_codecs.out
echo "exit code: 0" >> hub_codecs.log
exit $status
