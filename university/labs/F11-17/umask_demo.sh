#!/usr/bin/env bash
# F11-17 Listing 4: what the file-creation mask really does on this Linux machine.
# Creates files in a fresh temporary directory under two masks and prints their modes.
set -u
d="$(mktemp -d)"
for m in 0022 0077; do
    ( umask "$m"; : > "$d/made-with-$m.txt"; mkdir "$d/dir-made-with-$m" )
done
cd "$d"
stat -c '%A %a %n' made-with-0022.txt dir-made-with-0022 made-with-0077.txt dir-made-with-0077
cd /; rm -rf "$d"
