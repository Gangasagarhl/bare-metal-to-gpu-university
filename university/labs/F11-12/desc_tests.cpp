// desc_tests.cpp - F11-12: unit tests for the FIXED descriptor parser. Built and
// run by run_lab.sh under -fsanitize=address,undefined. The fixed parser must
// reject every malformed blob without reading past the buffer, and accept a
// well-formed one.
#define FIXED
#include "descparse.cc"      // brings in the fixed parse_config

#include <cstdio>
#include <vector>

namespace {
int failures = 0;
void expect(const char* what, int got, int want)
{
    bool ok = (want < 0) ? (got < 0) : (got == want);
    std::printf("%s: %s (got %d)\n", ok ? "ok" : "FAIL", what, got);
    failures += !ok;
}
}  // namespace

int main()
{
    using V = std::vector<unsigned char>;

    expect("empty blob rejected", parse_config(nullptr, 0), -1);

    // wTotalLength says 200 but only 4 bytes present.
    V lie{4, 2, 200, 0};
    expect("over-long wTotalLength rejected", parse_config(lie.data(), lie.size()), -1);

    // A descriptor with bLength 0 (would never advance).
    V zero{6, 2, 6, 0, 0, 9};
    expect("zero bLength rejected", parse_config(zero.data(), zero.size()), -1);

    // A descriptor whose bLength runs past the buffer.
    V past{6, 2, 6, 0, 99, 9};
    expect("bLength past end rejected", parse_config(past.data(), past.size()), -1);

    // A well-formed config: header(4) + two 2-byte descriptors, wTotalLength 8.
    V good{4, 2, 8, 0,  2, 5,  2, 5};
    expect("well-formed accepted (2 descriptors)", parse_config(good.data(), good.size()), 2);

    std::printf(failures ? "SOME TESTS FAILED\n" : "ALL TESTS PASSED\n");
    return failures;
}
