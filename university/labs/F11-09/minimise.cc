// minimise.cc - shrink a crashing input while it still crashes (F11-09).
// Usage: ./minimise <crash-in> <min-out>
// A fuzzer-found input is usually full of bytes that do not matter. A smaller
// input makes the bug obvious and makes a good regression test. This is the
// teaching version of ddmin (delta debugging): try to remove ever-smaller
// chunks of the input, keeping any removal that still crashes. It also trims the
// tail first, which collapses a large file quickly.
#include "fuzz.h"
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <sys/wait.h>
#include <unistd.h>

namespace {
using Bytes = std::vector<unsigned char>;

Bytes read_file(const char* path)
{
    Bytes b;
    FILE* f = std::fopen(path, "rb");
    if (!f) return b;
    unsigned char buf[4096];
    size_t n;
    while ((n = std::fread(buf, 1, sizeof buf, f)) > 0) b.insert(b.end(), buf, buf + n);
    std::fclose(f);
    return b;
}
void write_file(const char* path, const Bytes& b)
{
    FILE* f = std::fopen(path, "wb");
    if (!f) return;
    if (!b.empty()) std::fwrite(b.data(), 1, b.size(), f);
    std::fclose(f);
}

// True when the input still crashes the target (child dies abnormally).
bool crashes(const Bytes& in)
{
    pid_t p = fork();
    if (p == 0) {
        FILE* r = std::freopen("/dev/null", "w", stderr); (void)r;
        fuzz_one(in.data(), in.size());
        _exit(0);
    }
    int st = 0;
    waitpid(p, &st, 0);
    return !(WIFEXITED(st) && WEXITSTATUS(st) == 0);
}

Bytes without(const Bytes& b, size_t from, size_t len)
{
    Bytes t;
    t.reserve(b.size() - len);
    t.insert(t.end(), b.begin(), b.begin() + static_cast<long>(from));
    t.insert(t.end(), b.begin() + static_cast<long>(from + len), b.end());
    return t;
}
}  // namespace

int main(int argc, char** argv)
{
    if (argc < 3) { std::fprintf(stderr, "usage: %s <crash-in> <min-out>\n", argv[0]); return 2; }
    Bytes cur = read_file(argv[1]);
    if (!crashes(cur)) { std::printf("input does not crash; nothing to minimise\n"); return 1; }
    size_t start = cur.size();
    unsigned long tests = 0;

    // Pass 1: halve the tail as long as it still crashes.
    for (size_t cut = cur.size() / 2; cut >= 1; cut /= 2) {
        while (cur.size() > cut) {
            Bytes t(cur.begin(), cur.end() - static_cast<long>(cut));
            ++tests;
            if (crashes(t)) cur.swap(t); else break;
        }
    }

    // Pass 2: ddmin. Remove chunks, shrinking the chunk size when none can go.
    size_t chunk = cur.size() / 2;
    if (chunk == 0) chunk = 1;
    while (chunk >= 1) {
        bool removed = false;
        for (size_t at = 0; at < cur.size(); ) {
            size_t len = chunk < cur.size() - at ? chunk : cur.size() - at;
            if (len == cur.size()) break;                 // never remove everything
            Bytes t = without(cur, at, len);
            ++tests;
            if (crashes(t)) { cur.swap(t); removed = true; }   // keep 'at', the input shrank
            else at += len;
            if (tests > 20000) { removed = false; chunk = 1; break; }  // lab-time safety cap
        }
        if (!removed) { if (chunk == 1) break; chunk /= 2; }
    }

    write_file(argv[2], cur);
    std::printf("minimised %zu bytes -> %zu bytes in %lu tests (still crashes)\n",
                start, cur.size(), tests);
    return 0;
}
