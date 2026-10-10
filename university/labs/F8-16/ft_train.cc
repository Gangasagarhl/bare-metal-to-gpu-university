// F8-16 Listing 1: a data-parallel training job that detects a lost rank, aborts, and resumes.
//  - checkpoints: rank 0 writes the parameters and the step every `ckpt` steps to ckpt.tmp,
//    flushes it to disk, keeps the old checkpoint as ckpt.prev and renames ckpt.tmp to ckpt.bin;
//  - resume: at start every rank loads ckpt.bin, or ckpt.prev if ckpt.bin fails its checks;
//  - watchdog: the gradient all-reduce is non-blocking (MPI_Iallreduce) and polled; after `warn`
//    seconds a rank reports that it is still waiting, after `watchdog` seconds it calls MPI_Abort;
//  - trace: each rank appends its progress to rank<r>.trace, flushed line by line;
//  - fault injection: fail=<rank>:<step>:<hang|kill> freezes (SIGSTOP) or kills (SIGKILL) a rank.
// Arguments: steps=60 ckpt=10 watchdog=<s, 0 = off> warn=<s> fail=... (all optional)
#include <mpi.h>

#include <chrono>
#include <cmath>
#include <cstddef>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

constexpr int kP = 64;                 // parameters
constexpr int kBatch = 32;             // global batch
constexpr float kLr = 0.05f;
constexpr std::uint32_t kMagic = 0x4B434654;   // "TFCK"
using Clock = std::chrono::steady_clock;

std::uint64_t mix(std::uint64_t z)
{
    z += 0x9E3779B97F4A7C15ull;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

float uniform(std::uint64_t key) { return static_cast<float>(mix(key) >> 40) / 8388608.0f - 1.0f; }

std::uint64_t fnv1a(const void* data, std::size_t n)    // checksum of a byte range
{
    std::uint64_t h = 1469598103934665603ull;
    const auto* p = static_cast<const unsigned char*>(data);
    for (std::size_t i = 0; i < n; ++i) h = (h ^ p[i]) * 1099511628211ull;
    return h;
}

struct Checkpoint
{
    std::uint32_t magic = kMagic;
    std::int32_t nextStep = 0;         // the first step that has not been done yet
    float w[kP] = {};
    std::uint64_t sum = 0;             // fnv1a of all the fields above
};

bool readCheckpoint(const std::string& file, Checkpoint& c, std::string& why)
{
    std::FILE* f = std::fopen(file.c_str(), "rb");
    if (!f) { why = "missing"; return false; }
    const std::size_t got = std::fread(&c, 1, sizeof c, f);
    std::fclose(f);
    if (got != sizeof c) { why = "short file (" + std::to_string(got) + " of " + std::to_string(sizeof c) + " bytes)"; return false; }
    if (c.magic != kMagic) { why = "bad magic"; return false; }
    if (c.sum != fnv1a(&c, offsetof(Checkpoint, sum))) { why = "checksum mismatch"; return false; }
    return true;
}

void writeCheckpoint(const Checkpoint& c)
{
    std::FILE* f = std::fopen("ckpt.tmp", "wb");
    if (!f || std::fwrite(&c, sizeof c, 1, f) != 1 || std::fflush(f) != 0 || fsync(fileno(f)) != 0 ||
        std::fclose(f) != 0) {
        std::printf("rank 0: cannot write ckpt.tmp\n");
        MPI_Abort(MPI_COMM_WORLD, 5);
    }
    std::rename("ckpt.bin", "ckpt.prev");          // keep one older checkpoint (may not exist yet)
    if (std::rename("ckpt.tmp", "ckpt.bin") != 0) MPI_Abort(MPI_COMM_WORLD, 5);   // atomic replace
}

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);
    int rank = 0, size = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    int steps = 60, every = 10, failRank = -1, failStep = -1;
    double watchdog = 0.0, warn = 1.0;
    std::string failMode;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        const auto val = [&](const char* key) { return a.rfind(key, 0) == 0 ? a.c_str() + std::string(key).size() : nullptr; };
        if (const char* v = val("steps=")) steps = std::atoi(v);
        if (const char* v = val("ckpt=")) every = std::atoi(v);
        if (const char* v = val("watchdog=")) watchdog = std::atof(v);
        if (const char* v = val("warn=")) warn = std::atof(v);
        if (const char* v = val("fail=")) {
            char mode[8] = {};
            if (std::sscanf(v, "%d:%d:%7s", &failRank, &failStep, mode) == 3) failMode = mode;
        }
    }
    const auto t0 = Clock::now();
    const auto since = [&] { return std::chrono::duration<double>(Clock::now() - t0).count(); };
    std::FILE* trace = std::fopen(("rank" + std::to_string(rank) + ".trace").c_str(), "a");
    const auto note = [&](const std::string& s) {
        std::fprintf(trace, "t=%7.3f rank %d %s\n", since(), rank, s.c_str());
        std::fflush(trace);
    };

    Checkpoint c;
    std::string why1, why2;
    if (readCheckpoint("ckpt.bin", c, why1)) {
        if (rank == 0) std::printf("resuming from ckpt.bin at step %d\n", c.nextStep);
    } else if (readCheckpoint("ckpt.prev", c, why2)) {
        if (rank == 0) std::printf("ckpt.bin rejected (%s); resuming from ckpt.prev at step %d\n", why1.c_str(), c.nextStep);
    } else {
        c = Checkpoint{};
        if (rank == 0) std::printf("no usable checkpoint (ckpt.bin: %s); starting at step 0\n", why1.c_str());
    }
    note("start at step " + std::to_string(c.nextStep));
    const int shard = kBatch / size;

    for (int step = c.nextStep; step < steps; ++step) {
        note("step " + std::to_string(step) + " begin");
        if (rank == failRank && step == failStep) {           // fault injection (silent)
            if (failMode == "hang") std::raise(SIGSTOP);       // frozen: neither running nor exited
            if (failMode == "kill") std::raise(SIGKILL);       // gone at once, no message
        }
        std::vector<float> g(kP + 1, 0.0f);                  // gradient sum, then the loss sum
        for (int i = rank * shard; i < (rank + 1) * shard; ++i) {
            const std::uint64_t base = (static_cast<std::uint64_t>(step) << 32) + static_cast<std::uint64_t>(i) * kP;
            float pred = 0.0f, y = 0.0f;
            for (int j = 0; j < kP; ++j) {
                const float x = uniform(base + j);
                pred += c.w[j] * x;
                y += std::sin(0.1f * j) * x;
            }
            const float d = pred - y;
            for (int j = 0; j < kP; ++j) g[j] += d * uniform(base + j);
            g[kP] += 0.5f * d * d;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));   // stands in for GPU work

        MPI_Request req;
        MPI_Iallreduce(MPI_IN_PLACE, g.data(), kP + 1, MPI_FLOAT, MPI_SUM, MPI_COMM_WORLD, &req);
        note("step " + std::to_string(step) + " allreduce posted");
        const double posted = since();
        double nextWarn = warn;
        int done = 0;
        while (!done) {
            MPI_Test(&req, &done, MPI_STATUS_IGNORE);
            if (done) break;
            const double waited = since() - posted;
            if (waited >= nextWarn) {
                std::printf("t=%7.3f rank %d step %d: allreduce not complete after %.0f s\n", since(), rank, step, nextWarn);
                std::fflush(stdout);
                note("step " + std::to_string(step) + " still waiting");
                nextWarn += warn;
            }
            if (watchdog > 0 && waited >= watchdog) {
                std::printf("t=%7.3f rank %d step %d: WATCHDOG: no progress for %.1f s, calling MPI_Abort\n",
                            since(), rank, step, watchdog);
                std::fflush(stdout);
                note("watchdog abort");
                MPI_Abort(MPI_COMM_WORLD, 42);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        note("step " + std::to_string(step) + " allreduce done");
        for (int j = 0; j < kP; ++j) c.w[j] -= kLr / kBatch * g[j];
        if (rank == 0 && (step % 10 == 0 || step == steps - 1)) {
            std::printf("t=%7.3f step %2d loss %.6f\n", since(), step, g[kP] / kBatch);
        }
        if (rank == 0 && (step + 1) % every == 0) {
            c.nextStep = step + 1;
            c.sum = fnv1a(&c, offsetof(Checkpoint, sum));
            writeCheckpoint(c);
            std::printf("t=%7.3f checkpoint written: next step %d\n", since(), c.nextStep);
        }
    }
    if (rank == 0) {
        std::printf("finished %d steps; parameter checksum %016llx\n", steps,
                    static_cast<unsigned long long>(fnv1a(c.w, sizeof c.w)));
    }
    note("finished");
    std::fclose(trace);
    MPI_Finalize();
    return 0;
}
