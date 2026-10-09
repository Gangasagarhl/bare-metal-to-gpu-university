// spike.cpp - generates the evidence pack of the DS402 forensic lab "Latency spike at 14:02"
// (F5-41). The instrumented key-value service of Listing 1 runs from 14:00:00 to 14:05:00 at
// about 400 requests per second. One fault is injected; what it is, and when, is written only
// in the chapter's answer key. The generator prints what the service's telemetry would show:
// metrics at 10 s resolution, the slowest traces, and the logs of the gateway and the three
// nodes. Latencies come from an exercise model, not from measurements.
#include <algorithm>
#include <cstdio>
#include <map>
#include <random>
#include <string>
#include <vector>

struct Rng
{
    std::mt19937_64 g{1402};
    double uniform() { return static_cast<double>(g() >> 11) * 0x1.0p-53; }
    double between(double lo, double hi) { return lo + (hi - lo) * uniform(); }
};

// ---- the injected fault (see the answer key): node a's process stops running for a while
constexpr double kP0 = 127400.0;   // ms after 14:00:00
constexpr double kP1 = 128820.0;
constexpr double kTimeout = 1000.0;   // gateway timeout in ms

// work of duration d on node a, started at t: nothing on node a runs during the stop
double onA(double t, double d)
{
    if (t >= kP0 && t < kP1) {
        t = kP1;
    }
    double end = t + d;
    if (t < kP0 && end > kP0) {
        end += kP1 - kP0;
    }
    return end;
}

// a disk write issued by node a at t taking d at the device; the thread sees it complete
// only when it runs again
double fsyncOnA(double t, double d)
{
    if (t >= kP0 && t < kP1) {
        t = kP1;
    }
    double done = t + d;
    if (done >= kP0 && done < kP1) {
        done = kP1;
    }
    return done;
}

std::string hms(double ms)
{
    const long t = static_cast<long>(ms);
    char b[32];
    std::snprintf(b, sizeof b, "14:%02ld:%02ld.%03ld", t / 60000, (t / 1000) % 60, t % 1000);
    return b;
}

struct SpanRec
{
    std::string name;
    std::string node;
    double start;
    double end;
    int depth;
};

struct Req
{
    double arrive;
    double lat;
    bool put;
    std::vector<SpanRec> spans;
};

int main()
{
    Rng rng;
    std::vector<Req> reqs;
    std::vector<double> deviceAwait;   // per fsync on node a: (start time encoded separately)
    std::vector<double> deviceAt;
    double clock = 0;
    while (clock < 300000.0) {
        clock += rng.between(0.5, 4.5);
        Req r;
        r.arrive = clock;
        r.put = rng.uniform() < 0.2;
        double t = clock + rng.between(0.1, 0.3);
        const double leaderStart = t;
        if (r.put) {
            const double enc = onA(t, rng.between(0.02, 0.06));
            r.spans.push_back({"encode", "a", t, enc, 2});
            const double d = rng.uniform() < 0.01 ? rng.between(30, 80) : rng.between(0.8, 3.0);
            deviceAwait.push_back(d);
            deviceAt.push_back(enc);
            const double fs = fsyncOnA(enc, d);
            r.spans.push_back({"wal.fsync", "a", enc, fs, 2});
            const double send = onA(enc, 0.01);   // the replication message leaves node a
            double best = 1e18;
            for (const char* f : {"b", "c"}) {
                const double net = rng.between(0.2, 0.8);
                const double ffs = rng.uniform() < 0.01 ? rng.between(30, 80) : rng.between(0.8, 3.0);
                const double back = send + net + ffs + rng.between(0.2, 0.8);
                r.spans.push_back({"replicate", f, send, back, 2});
                best = std::min(best, back);
            }
            t = onA(std::max(fs, best), 0.02);   // the leader notices the majority
        } else {
            const bool miss = rng.uniform() < 0.05;
            const double d = miss ? rng.between(4, 12) : rng.between(0.05, 0.2);
            const double rd = miss ? fsyncOnA(t, d) : onA(t, d);
            r.spans.push_back({miss ? "store.read(disk)" : "store.read(memory)", "a", t, rd, 2});
            t = rd;
        }
        r.spans.insert(r.spans.begin(), {r.put ? "leader.put" : "leader.get", "a", leaderStart, t, 1});
        t += rng.between(0.1, 0.3);
        r.spans.insert(r.spans.begin(), {r.put ? "gateway put" : "gateway get", "gw", clock, t, 0});
        r.lat = t - clock;
        reqs.push_back(r);
    }

    // ---- metrics at 10 s resolution
    std::printf("=== metrics (10 s windows; latency measured at the gateway) ===\n");
    std::printf("window        req/s  errors(504)  p50 ms   p99 ms  max ms  a.disk_await_p99_ms  a.cpu_util  a.index_entries\n");
    long entries = 1000000;
    for (int w = 0; w < 30; ++w) {
        const double lo = w * 10000.0;
        const double hi = lo + 10000.0;
        std::vector<double> lat;
        int errors = 0;
        for (const auto& r : reqs) {
            if (r.arrive >= lo && r.arrive < hi) {
                lat.push_back(std::min(r.lat, kTimeout));
                errors += r.lat > kTimeout;
            }
        }
        std::sort(lat.begin(), lat.end());
        std::vector<double> dev;
        for (std::size_t i = 0; i < deviceAwait.size(); ++i) {
            if (deviceAt[i] >= lo && deviceAt[i] < hi) {
                dev.push_back(deviceAwait[i]);
            }
        }
        std::sort(dev.begin(), dev.end());
        const bool compaction = kP0 >= lo && kP0 < hi;
        // 4 cores; request work is small; the stop is spent by one thread working on the index
        const double cpu = 0.31 + 0.002 * static_cast<double>(w % 3) + (compaction ? (kP1 - kP0) / (hi - lo) / 4.0 : 0.0);
        entries += 1600 + 13 * (w % 4);
        if (compaction) {
            entries = 612331;
        }
        std::printf("%s  %5.0f  %11d  %6.2f  %7.2f  %6.0f  %19.2f  %10.2f  %15ld\n", hms(lo).c_str(),
                    static_cast<double>(lat.size()) / 10.0, errors, lat[lat.size() / 2],
                    lat[lat.size() * 99 / 100], lat.back(), dev[dev.size() * 99 / 100], cpu, entries);
    }

    // ---- the slowest traces, plus a traced get from the same seconds
    std::vector<std::size_t> order(reqs.size());
    for (std::size_t i = 0; i < order.size(); ++i) {
        order[i] = i;
    }
    std::sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) { return reqs[a].lat > reqs[b].lat; });
    std::vector<std::size_t> show(order.begin(), order.begin() + 2);
    for (std::size_t i : order) {
        if (!reqs[i].put && reqs[i].spans.back().name == "store.read(memory)") {
            show.push_back(i);   // the slowest get served from memory
            break;
        }
    }
    for (std::size_t i : order) {
        if (!reqs[i].put && reqs[i].arrive > kP1 + 2000 && reqs[i].arrive < kP1 + 4000) {
            show.push_back(i);   // the slowest get two seconds later, for comparison
            break;
        }
    }
    std::printf("\n=== traces (sampled: all requests slower than 500 ms are kept; 4 shown) ===\n");
    for (std::size_t i : show) {
        const Req& r = reqs[i];
        std::printf("trace %zu  arrived %s  total %.1f ms%s\n", i + 1, hms(r.arrive).c_str(), r.lat,
                    r.lat > kTimeout ? "  (gateway returned 504 at 1000 ms)" : "");
        for (const auto& s : r.spans) {
            std::printf("    %*s%-*s %-2s start +%8.2f ms  dur %8.2f ms\n", s.depth * 2, "", 20 - s.depth * 2,
                        s.name.c_str(), s.node.c_str(), s.start - r.arrive, s.end - s.start);
        }
    }

    // ---- logs
    std::printf("\n=== deploy.log ===\n");
    std::printf("%s gw: config reload (rate-limit table v41 -> v42), 0 errors\n", hms(90000).c_str());
    std::printf("\n=== gw.log (WARN and above, 14:01:00-14:03:00) ===\n");
    int shown = 0;
    int timeouts = 0;
    int slow500 = 0;
    double lastTimeout = 0;
    for (const auto& r : reqs) {
        if (r.arrive < 60000 || r.arrive > 180000) {
            continue;
        }
        slow500 += r.lat > 500.0 && r.arrive >= 120000 && r.arrive < 130000;
        if (r.lat > kTimeout) {
            ++timeouts;
            lastTimeout = r.arrive + kTimeout;
            if (shown < 3) {
                std::printf("%s WARN upstream a timed out after 1000 ms, returned 504\n", hms(r.arrive + kTimeout).c_str());
                ++shown;
            }
        }
    }
    std::printf("[... %d lines like these in total, the last at %s]\n", timeouts, hms(lastTimeout).c_str());
    std::printf("\n=== a.log (INFO and above, 14:02:05-14:02:12) ===\n");
    std::printf("%s INFO a: raft: commit index 4471290\n", hms(125800).c_str());
    std::printf("%s INFO a: index: starting compaction of the in-memory key index (entries %ld)\n",
                hms(127390).c_str(), 1000000L + 12 * 1600 + 13 * 18);
    std::printf("%s INFO a: index: compaction finished (entries 612331)\n", hms(kP1 + 0.4).c_str());
    std::printf("%s WARN a: raft: %.0f ms since last heartbeat sent to b, c (interval 100 ms)\n",
                hms(kP1 + 0.6).c_str(), kP1 - 100.0 * static_cast<long>(kP0 / 100.0));
    std::printf("%s WARN a: slow requests: %d requests waited more than 500 ms in the last 10 s\n",
                hms(kP1 + 1.2).c_str(), slow500);
    std::printf("%s INFO a: raft: commit index 4471801\n", hms(kP1 + 980.0).c_str());
    for (const char* f : {"b", "c"}) {
        std::printf("\n=== %s.log (14:02:05-14:02:12) ===\n", f);
        std::printf("%s WARN %s: raft: no heartbeat from leader a for 1000 ms (election timeout 1500 ms)\n",
                    hms(kP0 + 1000.0 + (f[0] == 'b' ? 3 : 1)).c_str(), f);
        std::printf("%s INFO %s: raft: heartbeat from leader a resumed (term 17)\n", hms(kP1 + 1.5).c_str(), f);
        std::printf("%s INFO %s: wal: fsync p99 over last 10 s: %.2f ms\n", hms(130000).c_str(), f,
                    f[0] == 'b' ? 2.94 : 2.97);
    }
    return 0;
}
