#!/usr/bin/env bash
# canon_readelf.sh FILE: turn GNU readelf's output into the same one-line-per-item form that
# elfread prints, so the two can be compared with diff. Only reformatting happens here:
# every value comes from readelf.
set -u -o pipefail
f="$1"
LC_ALL=C readelf -hW "$f" | awk -F: '
    function v(k) { return val[k] }
    { key=$1; sub(/^ +/, "", key); x=$0; sub(/^[^:]*: */, "", x); val[key]=x }
    END {
        split(v("Type"), t, " "); ty = (t[1]=="NONE")?0:(t[1]=="REL")?1:(t[1]=="EXEC")?2:(t[1]=="DYN")?3:(t[1]=="CORE")?4:t[1]
        split(v("Start of program headers"), a, " "); split(v("Start of section headers"), b, " ")
        split(v("Size of this header"), c, " "); split(v("Size of program headers"), d, " ")
        split(v("Size of section headers"), e, " ")
        sn = v("Number of section headers"); sub(/ .*/, "", sn)
        si = v("Section header string table index"); sub(/ .*/, "", si)
        printf "H type=%s machine=%s entry=%s phoff=%s shoff=%s flags=%s ehsize=%s phentsize=%s phnum=%s shentsize=%s shnum=%s shstrndx=%s\n",
            ty, v("Machine"), v("Entry point address"), a[1], b[1], v("Flags"), c[1], d[1],
            v("Number of program headers"), e[1], sn, si
    }'
LC_ALL=C readelf -SW "$f" | awk '
    /^  \[ *[0-9]+\]/ {
        line = $0; sub(/^  \[ */, "", line); nr = line; sub(/\].*/, "", nr); sub(/^[0-9]+\] */, "", line)
        n = split(line, F, " +")
        for (i = 1; i <= n; i++) if (F[i] ~ /^[0-9a-f]{16}$/) break
        name = (i >= 3) ? F[i-2] : ""; type = F[i-1]
        rest = n - (i + 3)
        flags = (rest == 4) ? F[i+4] : "-"
        printf "S %s %s %s %s %s %s %s %s %s %s %s\n", nr, name, type, F[i], F[i+1], F[i+2], F[i+3], flags, F[n-2], F[n-1], F[n]
    }'
LC_ALL=C readelf -lW "$f" | awk '
    /^Program Headers:/ { on = 1; next }
    /^ Section to Segment/ { on = 0 }
    on && /^  [A-Z]/ && $1 != "Type" {
        fl = ""; for (i = 7; i < NF; i++) fl = fl $i
        r = (fl ~ /R/) ? "R" : "-"; w = (fl ~ /W/) ? "W" : "-"; x = (fl ~ /E/) ? "E" : "-"
        printf "P %s %s %s %s %s %s %s%s%s %s\n", $1, $2, $3, $4, $5, $6, r, w, x, $NF
    }'
LC_ALL=C readelf -sW "$f" | awk '
    function hex2dec(h,   i, d) { d = 0; h = tolower(substr(h, 3)); for (i = 1; i <= length(h); i++) d = d * 16 + index("0123456789abcdef", substr(h, i, 1)) - 1; return sprintf("%.0f", d) }
    /^Symbol table / { t = $3; gsub(/\047/, "", t); next }
    /^ +[0-9]+:/ {
        num = $1; sub(/:/, "", num); name = ""
        if (NF >= 8) { name = $8; if (t == ".dynsym") sub(/@.*/, "", name) }
        size = $3; if (size ~ /^0x/) size = hex2dec(size)
        printf "Y %s %s %s %s %s %s %s %s %s\n", t, num, $2, size, $4, $5, $6, $7, name
    }'
