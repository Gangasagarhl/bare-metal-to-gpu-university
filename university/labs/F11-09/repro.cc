// repro.cc - run the target once, directly (no fork), on one input file, so the
// sanitizer prints its full report to stderr. Usage: ./repro <input>
// Built against the buggy target under -fsanitize=address,undefined; this is how
// the lab produces the "crash input" evidence (the minimised input plus its
// AddressSanitizer report) that the forensic lab works from.
#include "fuzz.h"
#include <cstdio>
#include <vector>

int main(int argc, char** argv)
{
    if (argc < 2) { std::fprintf(stderr, "usage: %s <input>\n", argv[0]); return 2; }
    std::vector<unsigned char> b;
    FILE* f = std::fopen(argv[1], "rb");
    if (!f) { std::perror("open"); return 2; }
    unsigned char buf[4096];
    size_t n;
    while ((n = std::fread(buf, 1, sizeof buf, f)) > 0) b.insert(b.end(), buf, buf + n);
    std::fclose(f);
    std::printf("replaying %zu bytes through the parser\n", b.size());
    std::fflush(stdout);
    return fuzz_one(b.data(), b.size());
}
