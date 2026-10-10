#!/usr/bin/env bash
# mkinitrd.sh - F3-31: build the initial RAM disk tree and pack it as a USTAR archive.
#   mkinitrd.sh <output.tar>
set -eu
out="$1"
root="$(mktemp -d)"
mkdir -p "$root/etc" "$root/docs/guides" "$root/sys" "$root/bin"
printf 'Welcome to the OS304 mini kernel.\nThis file came from the initial RAM disk.\n' > "$root/etc/motd"
printf 'os304-qemu\n' > "$root/etc/hostname"
seq 1 1000 > "$root/docs/numbers.txt"                 # 3893 bytes: crosses many 512-byte blocks
printf 'Programs arrive in F3-35.\n' > "$root/bin/README"
deep="docs/guides/a-rather-long-directory-name-for-testing/another-long-directory-name-inside"
mkdir -p "$root/$deep"
printf 'found me\n' > "$root/$deep/a-file-whose-full-path-is-longer-than-one-hundred-characters.txt"
chmod 0755 "$root"/docs "$root"/etc "$root"/sys "$root"/bin "$root"/docs/guides "$root"/docs/guides/*/ "$root/$deep"
chmod 0644 "$root"/etc/* "$root"/docs/numbers.txt "$root"/bin/README "$root/$deep"/*
chmod 0600 "$root/etc/hostname"
tar --format=ustar --sort=name --owner=0 --group=0 --numeric-owner --mtime=@0 \
    -C "$root" -cf "$out" bin docs etc sys
rm -rf "$root"
