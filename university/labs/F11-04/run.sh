#!/usr/bin/env bash
# F11-04 lab: signed firmware updates on a modelled two-slot device (host program; no
# microcontroller, MCUboot or imgtool is available in this build).
#   updates    keys, the H4 acceptance tests, rollback, power-cut sweep, unconfirmed image, key rotation
#   crosscheck OpenSSL's command-line tool verifies an image signature made by fwimage.h
#   forensic   the evidence pack of the forensic lab
set -u -o pipefail
cd "$(dirname "$0")"
status=0
B=.build
rm -rf "$B"; mkdir -p "$B" keys
rec() {  # rec <name> <listing> <toolchain> <command> <exit code> [extra line]
    {
        echo "listing:   $2"
        echo "toolchain: $3"
        echo "command:   $4"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
        echo "exit code: $5"
        if [ $# -ge 6 ]; then echo "$6"; fi
    } > "$1.log"
}
FLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
TC="$(g++ --version | head -n 1); $(openssl version)"
NOTE="hardware:  untested on hardware; a host model of a two-slot device, not MCUboot and not a microcontroller"
g++ $FLAGS updates.cc -lcrypto -o $B/updates > $B/build.txt 2>&1 || { cat $B/build.txt; status=1; }
$B/updates demo > updates.out 2>&1; rc=$?
rec updates "fwimage.h updates.cc" "$TC" "g++ $FLAGS updates.cc -lcrypto -o updates && ./updates demo" "$rc" "$NOTE"
[ "$rc" = 0 ] || status=1
grep -q "0 ran nothing" updates.out || status=1
[ "$(grep -c 'update REFUSED' updates.out)" -ge 5 ] || status=1

# an independent check of the signature format: sign with the C++ code, verify with openssl
cat > $B/one.cc <<'CC'
#include <fstream>
#include "../fwimage.h"
int main()
{
    fw::Pkey a = fw::load_or_make_key("vendor_a");
    const fw::Bytes img = fw::build_image("app 1.1", 1, 1, 1, a.get());
    const auto p = fw::parse(img);
    std::ofstream("signed_region.bin", std::ios::binary).write(reinterpret_cast<const char*>(p->signed_region.data()), long(p->signed_region.size()));
    const fw::Bytes* s = p->find(fw::kTlvSignature);
    std::ofstream("sig.der", std::ios::binary).write(reinterpret_cast<const char*>(s->data()), long(s->size()));
    return 0;
}
CC
g++ $FLAGS -I. $B/one.cc -lcrypto -o $B/one && (cd $B && ln -s ../keys keys && ./one) || status=1
{
    echo "openssl: public key of keys/vendor_a.pem, then verify the image's signature over header+payload"
    openssl pkey -in keys/vendor_a.pem -pubout -out $B/a.pub && \
    openssl dgst -sha256 -verify $B/a.pub -signature $B/sig.der $B/signed_region.bin
    echo "openssl: the same signature against header+payload with one byte changed"
    printf '\x01' | dd of=$B/signed_region.bin bs=1 seek=36 conv=notrunc status=none
    openssl dgst -sha256 -verify $B/a.pub -signature $B/sig.der $B/signed_region.bin
    true
} > crosscheck.out 2>&1; rc=$?
rec crosscheck "fwimage.h (a five-line driver in run.sh writes the signed region and the signature)" "$TC" \
    "openssl pkey -in keys/vendor_a.pem -pubout; openssl dgst -sha256 -verify a.pub -signature sig.der signed_region.bin (then with one byte changed)" "$rc"
grep -q "^Verified OK" crosscheck.out && grep -q "Verification failure" crosscheck.out || status=1

$B/updates forensic > forensic.out 2>&1; rc=$?
rec forensic "fwimage.h updates.cc" "$TC" "./updates forensic" "$rc" "$NOTE"
[ "$rc" = 0 ] || status=1
rm -rf "$B"
exit $status
