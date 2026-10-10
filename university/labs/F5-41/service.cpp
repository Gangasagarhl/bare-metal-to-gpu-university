// service.cpp - a replicated key-value service, simulated in one process and instrumented with
// the three kinds of telemetry (F5-41, Listing 1): structured logs, metrics (counters and a
// latency histogram) and traces (spans with trace id, span id and parent). It stands in for the
// DS302 Raft key-value service: a gateway forwards to the leader (node a); a put is written to
// the leader's write-ahead log and replicated to followers b and c, and is acknowledged when a
// majority (the leader and one follower) has it. Times are simulated milliseconds from an
// exercise latency model, not measurements.
#include <algorithm>
#include <cstdio>
#include <map>
#include <random>
#include <string>
#include <vector>

struct Rng
{
    std::mt19937_64 g{41};
    double uniform() { return static_cast<double>(g() >> 11) * 0x1.0p-53; }
    double between(double lo, double hi) { return lo + (hi - lo) * uniform(); }
};

// ---- tracing: a span is one timed step of one request
struct Span
{
    int trace;
    int id;
    int parent;   // 0 = root
    std::string name;
    std::string node;
    double start;
    double dur;
};

struct Tracer
{
    std::vector<Span> spans;
    int nextId = 1;
    int begin(int trace, int parent, const std::string& name, const std::string& node, double start)
    {
        spans.push_back({trace, nextId, parent, name, node, start, 0.0});
        return nextId++;
    }
    void end(int id, double t) { spans[static_cast<std::size_t>(id - 1)].dur = t - spans[static_cast<std::size_t>(id - 1)].start; }
};

// ---- metrics: counters and a fixed-bucket histogram
struct Histogram
{
    std::vector<double> bounds{0.5, 1, 2, 5, 10, 20, 50, 100};   // upper bounds in ms; last = +Inf
    std::vector<long> counts = std::vector<long>(bounds.size() + 1, 0);
    double sum = 0;
    long n = 0;
    void observe(double v)
    {
        std::size_t i = 0;
        while (i < bounds.size() && v > bounds[i]) {
            ++i;
        }
        ++counts[i];
        sum += v;
        ++n;
    }
    // estimate a quantile from bucket counts by linear interpolation inside the bucket
    double quantile(double q) const
    {
        const double rank = q * static_cast<double>(n);
        long cum = 0;
        for (std::size_t i = 0; i < counts.size(); ++i) {
            if (static_cast<double>(cum + counts[i]) >= rank) {
                const double lo = i == 0 ? 0.0 : bounds[i - 1];
                const double hi = i < bounds.size() ? bounds[i] : bounds.back();
                return lo + (hi - lo) * (rank - static_cast<double>(cum)) / static_cast<double>(counts[i]);
            }
            cum += counts[i];
        }
        return bounds.back();
    }
};

double exactQuantile(std::vector<double> v, double q)
{
    std::sort(v.begin(), v.end());
    const std::size_t i = static_cast<std::size_t>(q * static_cast<double>(v.size() - 1));
    return v[i];
}

int main()
{
    Rng rng;
    Tracer tr;
    Histogram hist;
    std::map<std::string, long> counter;   // requests_total by op
    std::vector<double> raw;
    std::vector<std::string> logs;
    std::vector<std::pair<double, int>> byLatency;   // (latency, trace id)
    const int kRequests = 2000;
    double clock = 0;

    for (int trace = 1; trace <= kRequests; ++trace) {
        clock += rng.between(0.5, 4.5);   // about 400 requests per simulated second
        const bool isPut = rng.uniform() < 0.2;
        const std::string op = isPut ? "put" : "get";
        const int key = static_cast<int>(rng.uniform() * 500);
        double t = clock;
        const int root = tr.begin(trace, 0, "gateway " + op, "gw", t);
        t += rng.between(0.1, 0.3);
        const int leader = tr.begin(trace, root, "leader." + op, "a", t);
        if (isPut) {
            const int wal = tr.begin(trace, leader, "wal.fsync", "a", t);
            const double fs = rng.uniform() < 0.01 ? rng.between(30, 80) : rng.between(0.8, 3.0);
            tr.end(wal, t + fs);
            double followerDone[2];
            for (int f = 0; f < 2; ++f) {
                const std::string node = f == 0 ? "b" : "c";
                const int rep = tr.begin(trace, leader, "replicate", node, t);
                const double net = rng.between(0.2, 0.8);
                const int fw = tr.begin(trace, rep, "wal.fsync", node, t + net);
                const double ffs = rng.uniform() < 0.01 ? rng.between(30, 80) : rng.between(0.8, 3.0);
                tr.end(fw, t + net + ffs);
                followerDone[f] = t + net + ffs + rng.between(0.2, 0.8);
                tr.end(rep, followerDone[f]);
            }
            // majority: the leader's own log plus the faster follower
            t = std::max(t + fs, std::min(followerDone[0], followerDone[1]));
        } else {
            const bool miss = rng.uniform() < 0.05;
            const int rd = tr.begin(trace, leader, miss ? "store.read(disk)" : "store.read(memory)", "a", t);
            t += miss ? rng.between(4, 12) : rng.between(0.05, 0.2);
            tr.end(rd, t);
        }
        tr.end(leader, t);
        t += rng.between(0.1, 0.3);
        tr.end(root, t);
        const double lat = t - clock;
        hist.observe(lat);
        raw.push_back(lat);
        counter[op] += 1;
        byLatency.push_back({lat, trace});
        if (trace <= 3 || lat > 30) {
            char b[200];
            std::snprintf(b, sizeof b,
                          "ts=%.1f level=%s node=a msg=\"%s done\" key=k%d trace=%d dur_ms=%.2f", clock,
                          lat > 30 ? "WARN" : "INFO", op.c_str(), key, trace, lat);
            logs.push_back(b);
        }
    }

    std::printf("== 1. structured logs (first 3 requests and every request slower than 30 ms)\n");
    for (std::size_t i = 0; i < logs.size() && i < 9; ++i) {
        std::printf("%s\n", logs[i].c_str());
    }
    std::printf("[... %zu log lines in total]\n", logs.size());

    std::sort(byLatency.rbegin(), byLatency.rend());
    const int slow = byLatency.front().second;
    std::printf("\n== 2. the trace of the slowest request (trace %d), as a waterfall\n", slow);
    double t0 = 0;
    for (const auto& s : tr.spans) {
        if (s.trace == slow && s.parent == 0) {
            t0 = s.start;
        }
    }
    for (const auto& s : tr.spans) {
        if (s.trace != slow) {
            continue;
        }
        int depth = 0;
        for (int p = s.parent; p != 0; p = tr.spans[static_cast<std::size_t>(p - 1)].parent) {
            ++depth;
        }
        const int from = static_cast<int>((s.start - t0) / 2.0);
        const int len = std::max(1, static_cast<int>(s.dur / 2.0));
        std::printf("span %5d parent %5d  %-*s%-*s %-2s %7.2f ms |%*s%s\n", s.id, s.parent, depth * 2, "",
                    20 - depth * 2, s.name.c_str(), s.node.c_str(), s.dur, from, "",
                    std::string(static_cast<std::size_t>(len), '#').c_str());
    }
    std::printf("(indentation follows the parent links; each # is 2 ms of the request's time)\n");

    std::printf("\n== 3. metrics at the end of the run\n");
    for (const auto& [op, n] : counter) {
        std::printf("requests_total{op=\"%s\"} %ld\n", op.c_str(), n);
    }
    long cum = 0;
    for (std::size_t i = 0; i < hist.counts.size(); ++i) {
        cum += hist.counts[i];
        if (i < hist.bounds.size()) {
            std::printf("latency_ms_bucket{le=\"%g\"} %ld\n", hist.bounds[i], cum);
        } else {
            std::printf("latency_ms_bucket{le=\"+Inf\"} %ld\n", cum);
        }
    }
    std::printf("latency_ms_sum %.1f\nlatency_ms_count %ld\n", hist.sum, hist.n);

    std::printf("\n== 4. what the numbers say (all requests)\n");
    std::printf("mean %.2f ms\n", hist.sum / static_cast<double>(hist.n));
    for (double q : {0.5, 0.9, 0.99, 0.999}) {
        std::printf("p%-5g exact %7.2f ms   estimated from buckets %7.2f ms\n", q * 100, exactQuantile(raw, q),
                    hist.quantile(q));
    }

    std::printf("\n== 5. sampling: keep 1 trace in 10, decided when the request starts\n");
    int kept = 0;
    int keptSlow = 0;
    const std::size_t slowCount = byLatency.size() / 100;   // the slowest 1 %
    for (std::size_t i = 0; i < byLatency.size(); ++i) {
        if (byLatency[i].second % 10 == 0) {
            ++kept;
            keptSlow += i < slowCount;
        }
    }
    std::printf("traces kept %d of %d; of the %zu slowest requests, traces kept: %d\n", kept, kRequests,
                slowCount, keptSlow);

    std::printf("\n== 6. label cardinality: number of time series for requests_total\n");
    std::printf("labels {op}: %zu series; labels {op, key}: up to %d series\n", counter.size(), 2 * 500);
    return 0;
}
