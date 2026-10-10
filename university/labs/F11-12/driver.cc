// driver.cc - F11-12: read a descriptor blob from a file and parse it. Usage:
// ./driver <file>. Built against the buggy or the fixed parser by run.sh.
#include "shared.h"
#include <cstdio>
#include <vector>

int main(int argc, char** argv)
{
    if (argc < 2) { std::fprintf(stderr, "usage: %s <file>\n", argv[0]); return 2; }
    std::vector<unsigned char> b;
    FILE* f = std::fopen(argv[1], "rb");
    if (!f) { std::perror("open"); return 2; }
    unsigned char buf[4096]; size_t n;
    while ((n = std::fread(buf, 1, sizeof buf, f)) > 0) b.insert(b.end(), buf, buf + n);
    std::fclose(f);
    std::printf("parsing %zu bytes from the device\n", b.size());
    std::fflush(stdout);
    int r = parse_config(b.data(), b.size());
    if (r < 0) std::printf("rejected: malformed descriptor\n");
    else       std::printf("accepted: %d descriptors\n", r);
    return r < 0 ? 1 : 0;
}
