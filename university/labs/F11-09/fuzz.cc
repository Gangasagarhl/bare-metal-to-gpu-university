// fuzz.cc - the fuzzing loop (F11-09). Usage:
//     ./fuzz <seconds> <crash-dir> <seed-file>...
// It keeps a corpus (the seeds plus every input that reached a new edge),
// mutates a random corpus entry, runs the target in a forked child, and:
//   - if the child died (signal or non-zero exit) it saves the input to
//     <crash-dir>/crash-NNN.bin, copies the child's sanitizer report, and stops;
//   - otherwise, if the run lit a new edge in the shared bitmap, the input
//     joins the corpus.
// The RNG seed is fixed (12345) so a run can be repeated. Compiled WITHOUT
// coverage instrumentation.
#include "fuzz.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>
#include <sys/wait.h>
#include <unistd.h>

namespace {

using Bytes = std::vector<unsigned char>;

uint64_t g_rng = 12345;
uint64_t rnd()                       // xorshift64, deterministic
{
    g_rng ^= g_rng << 13;
    g_rng ^= g_rng >> 7;
    g_rng ^= g_rng << 17;
    return g_rng;
}
unsigned pick(unsigned n) { return n == 0 ? 0 : static_cast<unsigned>(rnd() % n); }

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

void write_file(const std::string& path, const Bytes& b)
{
    FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) return;
    if (!b.empty()) std::fwrite(b.data(), 1, b.size(), f);
    std::fclose(f);
}

// Five classic mutation kinds, biased to the first 64 bytes where headers and
// length fields of these parsers live.
void mutate(Bytes& b)
{
    if (b.empty()) b.push_back(0);
    unsigned kind = pick(5);
    unsigned hot = b.size() < 64 ? b.size() : 64;
    unsigned at = pick(2) ? pick(hot) : pick(static_cast<unsigned>(b.size()));
    switch (kind) {
        case 0: b[at] ^= static_cast<unsigned char>(1u << pick(8)); break;     // bit flip
        case 1: b[at] = static_cast<unsigned char>(rnd()); break;              // random byte
        case 2: {                                                              // interesting value
            static const unsigned char iv[] = {0, 1, 0x7f, 0x80, 0xff};
            b[at] = iv[pick(sizeof iv)];
            break;
        }
        case 3:                                                                // truncate
            if (b.size() > 1) b.resize(1 + pick(static_cast<unsigned>(b.size() - 1)));
            break;
        case 4:                                                                // grow by a copied byte
            if (b.size() < 4096) b.insert(b.begin() + at, b[pick(static_cast<unsigned>(b.size()))]);
            break;
    }
}

// Run the target once in a child. Returns true if the child finished cleanly.
// On a crash, the child's sanitizer report is left in report_path.
bool run_child(const Bytes& in, const char* report_path, int& status)
{
    pid_t p = fork();
    if (p == 0) {
        FILE* r = std::freopen(report_path, "w", stderr); (void)r;  // capture the report
        fuzz_one(in.data(), in.size());
        _exit(0);
    }
    waitpid(p, &status, 0);
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

unsigned count_edges()
{
    unsigned n = 0;
    for (unsigned i = 0; i < g_cov_size; ++i) n += (g_cov[i] != 0);
    return n;
}

}  // namespace

int main(int argc, char** argv)
{
    if (argc < 4) {
        std::fprintf(stderr, "usage: %s <seconds> <crash-dir> <seed>...\n", argv[0]);
        return 2;
    }
    long seconds = std::atol(argv[1]);
    std::string crashdir = argv[2];
    std::string report = crashdir + "/last-report.txt";

    cov_init();

    std::vector<Bytes> corpus;
    for (int i = 3; i < argc; ++i) {
        Bytes b = read_file(argv[i]);
        if (!b.empty()) corpus.push_back(std::move(b));
    }
    if (corpus.empty()) corpus.push_back(Bytes{0});

    // Prime the bitmap with the seeds so we only keep inputs with NEW edges.
    for (const Bytes& s : corpus) { int st; run_child(s, report.c_str(), st); }

    unsigned long execs = 0;
    unsigned edges0 = count_edges();
    time_t start = std::time(nullptr);
    int found = 0;

    while (std::time(nullptr) - start < seconds) {
        Bytes cand = corpus[pick(static_cast<unsigned>(corpus.size()))];
        unsigned rounds = 1 + pick(4);
        for (unsigned r = 0; r < rounds; ++r) mutate(cand);

        unsigned before = count_edges();
        int status = 0;
        bool ok = run_child(cand, report.c_str(), status);
        ++execs;

        if (!ok) {
            char name[256];
            std::snprintf(name, sizeof name, "%s/crash-%03d.bin", crashdir.c_str(), found);
            write_file(name, cand);
            std::printf("CRASH after %lu executions: input saved to %s (%zu bytes)\n",
                        execs, name, cand.size());
            if (WIFSIGNALED(status))
                std::printf("  child killed by signal %d\n", WTERMSIG(status));
            else
                std::printf("  child exit code %d (sanitizer exitcode)\n", WEXITSTATUS(status));
            ++found;
            break;                     // stop at the first crash: now minimise and fix
        }
        if (count_edges() > before) corpus.push_back(cand);
    }

    std::printf("executions:   %lu\n", execs);
    std::printf("corpus:       %zu inputs\n", corpus.size());
    std::printf("edges:        %u (started at %u)\n", count_edges(), edges0);
    std::printf("crashes:      %d\n", found);
    std::printf("result:       %s\n", found ? "a crash was found (see the saved input and report)"
                                            : "no crash, no sanitizer report in this run");
    return 0;
}
