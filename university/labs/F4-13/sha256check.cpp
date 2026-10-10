// sha256check.cpp - DR302 F4-13: sha256.h against the example digests of FIPS 180-4
// (copied here from memory: run.sh also checks sha256.h against Python's hashlib), plus
// the padding edge cases (55, 56, 63, 64 and 65 bytes) and one PCR extend worked by hand.
#include "sha256.h"
#include <cstdio>
#include <cstring>
#include <string>

namespace {
std::string hex(const uint8_t* d, size_t n)
{
    static const char* x = "0123456789abcdef";
    std::string s;
    for (size_t i = 0; i < n; ++i) { s += x[d[i] >> 4]; s += x[d[i] & 15]; }
    return s;
}
int failures = 0;
void vec(const char* label, const std::string& msg, const char* want)
{
    uint8_t d[32];
    sha::hash(msg.data(), msg.size(), d);
    const std::string got = hex(d, 32);
    const bool ok = want == nullptr || got == want;
    if (!ok) ++failures;
    std::printf("%-30s %s %s\n", label, got.c_str(), want == nullptr ? "" : ok ? "ok" : "MISMATCH");
}
}  // namespace

int main()
{
    vec("\"\" (empty)", "", "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    vec("\"abc\"", "abc", "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    vec("448-bit two-block message", "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
        "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
    vec("one million 'a'", std::string(1000000, 'a'),
        "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0");
    // Padding edge cases: run.sh compares these lines with hashlib.
    for (size_t n : {55u, 56u, 63u, 64u, 65u}) {
        char label[32];
        std::snprintf(label, sizeof label, "%zu x 'x'", n);
        vec(label, std::string(n, 'x'), nullptr);
    }
    // PCR extend: PCR starts at 32 zero bytes; extend with SHA-256("abc").
    uint8_t pcr[32] = {}, d[32];
    sha::hash("abc", 3, d);
    sha::extend(pcr, d);
    std::printf("%-30s %s\n", "extend(0^32, sha256(abc))", hex(pcr, 32).c_str());
    std::printf("%s: %d mismatch(es) with the FIPS 180-4 examples\n", failures ? "FAIL" : "PASS", failures);
    return failures ? 1 : 0;
}
