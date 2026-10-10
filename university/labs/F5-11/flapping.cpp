// flapping.cpp - generates the evidence pack of the F5-11 forensic lab "the database that died
// and came back". A cluster monitor watches db2's heartbeats (one every 100 ms) with a fixed
// timeout; when it declares db2 dead it promotes db3. The planted cause is in the answer key.
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

struct Rng
{
    std::uint64_t state;
    std::uint64_t next()
    {
        std::uint64_t z = (state += 0x9E3779B97F4A7C15ULL);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    }
    double uniform() { return static_cast<double>(next() >> 11) * 0x1.0p-53; }
};

std::string at(double ms)   // wall clock, all machines synchronised in this scenario
{
    const int t = static_cast<int>(ms) + (14 * 3600 + 2 * 60) * 1000;
    char buf[32];
    std::snprintf(buf, sizeof buf, "14:%02d:%02d.%03d", t / 60000 % 60, t / 1000 % 60, t % 1000);
    return buf;
}

int main()
{
    Rng rng{5};
    const double timeoutMs = 500.0;
    std::vector<std::string> dbLog;
    std::vector<double> arrivals;
    double stallUntil = -1;
    // db2: heartbeat thread and checkpoint share one lock; every 30 s a checkpoint holds it
    for (double t = 0; t < 120000; t += 100) {
        if (static_cast<int>(t) % 30000 == 15000) {
            const double len = 450 + 500 * rng.uniform();
            dbLog.push_back(at(t) + " db2 checkpoint started (dirty pages: "
                            + std::to_string(2000 + static_cast<int>(6000 * rng.uniform())) + ")");
            dbLog.push_back(at(t + len) + " db2 checkpoint finished in "
                            + std::to_string(static_cast<int>(len)) + " ms");
            stallUntil = t + len;
            if (t == 15000) {   // db2 carries on as before: nobody has told it anything
                dbLog.push_back(at(t + len + 166) + " db2 commit txn 88121 (role: primary)");
            }
        }
        double send = t;
        if (send < stallUntil) {
            continue;              // the heartbeat waits for the lock... and is skipped
        }
        arrivals.push_back(send + 1.0 + 2.0 * rng.uniform());
    }
    std::printf("=== monitor.log (timeout %.0f ms) ===\n", timeoutMs);
    bool primaryIsDb2 = true;
    for (std::size_t i = 1; i < arrivals.size(); ++i) {
        const double gap = arrivals[i] - arrivals[i - 1];
        if (gap > timeoutMs) {
            std::printf("%s monitor: db2 no heartbeat for %.0f ms -> db2 DEAD\n",
                        at(arrivals[i - 1] + timeoutMs).c_str(), timeoutMs);
            if (primaryIsDb2) {
                std::printf("%s monitor: failover: promoting db3 to primary\n",
                            at(arrivals[i - 1] + timeoutMs + 3).c_str());
                primaryIsDb2 = false;
            }
            std::printf("%s monitor: db2 heartbeat received -> db2 ALIVE (as replica)\n",
                        at(arrivals[i]).c_str());
        }
    }
    std::printf("\n=== db2.log (excerpt) ===\n");
    for (const std::string& line : dbLog) {
        std::printf("%s\n", line.c_str());
    }
    std::printf("\n=== db3.log (excerpt) ===\n");
    std::printf("%s db3 promoted to primary by monitor; accepting writes\n", at(15420).c_str());
    std::printf("%s db3 commit txn 88121 (role: primary)\n", at(15688).c_str());
    std::printf("\n=== db2 configuration (excerpt) ===\n");
    std::printf("heartbeat_interval_ms = 100\n");
    std::printf("checkpoint_interval_s = 30\n");
    std::printf("heartbeat and checkpoint run on the same worker thread = yes\n");
    return 0;
}
