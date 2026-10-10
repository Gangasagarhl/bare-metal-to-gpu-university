// kvsvc.cc - F12-14 Listing 1: a small, REAL multi-threaded key-value service, instrumented
// for a whole-system profile. It stands in for the MP2 replicated key-value service on one
// machine: no network and no replication, but the same kinds of parts: a request queue,
// worker threads, one store lock, a write-ahead log with fdatasync, and background tasks.
//
//   generator --> request queue --> 3 workers --> [store lock] --> store
//                                            \--> write-ahead log (puts wait for fdatasync)
//   background: snapshot task (every 300 ms), metrics task (every 1 s)
//
// Every request records timestamps at each step (a trace with spans); every thread records
// its CPU time and context switches; background tasks write a log with timestamps.
// --mode whole   : the snapshot holds the store lock while it formats the whole store.
// --mode chunked : the snapshot takes the lock for 1,000 entries at a time, back to back.
// --mode paced   : like chunked, but it sleeps 0.2 ms between chunks so waiting workers get in.
// Output: the evidence report on stdout; with --key FILE the analysis is written to FILE.
#include <sys/resource.h>
#include <fcntl.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using Clock = std::chrono::steady_clock;
Clock::time_point g_t0;
double now_us()
{
    return std::chrono::duration<double, std::micro>(Clock::now() - g_t0).count();
}

struct Rng
{
    std::uint64_t s;
    std::uint64_t next()
    {
        std::uint64_t z = (s += 0x9E3779B97F4A7C15ULL);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    }
    double uniform() { return static_cast<double>(next() >> 11) * 0x1.0p-53; }
    double exponential(double mean) { return -mean * std::log(1.0 - uniform()); }
};

// ---- one request and its trace (all times in microseconds since start) ----
struct Request
{
    int id = 0;
    bool put = false;
    std::uint32_t key = 0;
    double intended = 0, dequeued = 0, parsed = 0, lock_got = 0, lock_rel = 0, durable = 0,
           done = 0;
};

// ---- per-thread resource record (the "U" and part of the "S" of the USE method) ----
struct ThreadStat
{
    std::string name;
    double wall_ms = 0, cpu_ms = 0;
    long vol_cs = 0, invol_cs = 0;
};
std::mutex g_stats_mu;
std::vector<ThreadStat> g_stats;
void record_thread(std::string const& name, double start_us)
{
    timespec ts{};
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts);
    rusage ru{};
    getrusage(RUSAGE_THREAD, &ru);
    std::lock_guard lock(g_stats_mu);
    g_stats.push_back({name, (now_us() - start_us) / 1000.0,
                       static_cast<double>(ts.tv_sec) * 1e3 + static_cast<double>(ts.tv_nsec) / 1e6,
                       ru.ru_nvcsw, ru.ru_nivcsw});
}

// ---- the service ----
constexpr std::size_t kKeys = 100000;
struct Service
{
    std::mutex store_mu;  // THE store lock
    std::vector<std::array<char, 64>> store = std::vector<std::array<char, 64>>(kKeys);

    std::mutex q_mu;
    std::condition_variable q_cv;
    std::deque<Request*> queue;
    bool closing = false;

    std::mutex wal_mu;
    std::condition_variable wal_cv;
    std::string wal_buffer;
    std::uint64_t wal_appended = 0, wal_durable = 0;
    int wal_fd = -1;

    std::mutex log_mu;
    std::vector<std::string> bg_log;                        // background-task log lines
    std::vector<std::pair<double, double>> snapshot_holds;  // [start, end) of each lock hold
    std::atomic<bool> stop{false};
};

void bg_log(Service& s, std::string line)
{
    std::lock_guard lock(s.log_mu);
    s.bg_log.push_back(std::move(line));
}

std::uint64_t cpu_work(std::uint32_t seed, int rounds)  // stands for parsing / formatting
{
    std::uint64_t h = seed;
    for (int i = 0; i < rounds; ++i) {
        h = (h ^ (h >> 29)) * 0xBF58476D1CE4E5B9ULL + static_cast<std::uint64_t>(i);
    }
    return h;
}

void worker(Service& s, std::string name)
{
    double const start = now_us();
    std::uint64_t sink = 0;
    for (;;) {
        Request* r = nullptr;
        {
            std::unique_lock lock(s.q_mu);
            s.q_cv.wait(lock, [&] { return s.closing || !s.queue.empty(); });
            if (s.queue.empty()) {
                break;
            }
            r = s.queue.front();
            s.queue.pop_front();
        }
        r->dequeued = now_us();
        sink += cpu_work(r->key, 2000);  // parse the request
        r->parsed = now_us();
        std::uint64_t seq = 0;
        {
            std::lock_guard lock(s.store_mu);
            r->lock_got = now_us();
            auto& value = s.store[r->key];
            if (r->put) {
                std::snprintf(value.data(), value.size(), "v%d", r->id);
            } else {
                sink += static_cast<unsigned char>(value[1]);
            }
            r->lock_rel = now_us();
        }
        if (r->put) {
            std::unique_lock lock(s.wal_mu);
            s.wal_buffer += "put " + std::to_string(r->key) + " v" + std::to_string(r->id) + "\n";
            seq = ++s.wal_appended;
            s.wal_cv.wait(lock, [&] { return s.wal_durable >= seq; });
        }
        r->durable = now_us();
        sink += cpu_work(r->key, 1000);  // format the reply
        r->done = now_us();
    }
    if (sink == 42) {
        std::printf("(unlikely)\n");
    }
    record_thread(name, start);
}

void wal_writer(Service& s)
{
    double const start = now_us();
    std::string batch;
    for (;;) {
        std::uint64_t upto = 0;
        {
            std::unique_lock lock(s.wal_mu);
            s.wal_cv.wait_for(lock, std::chrono::milliseconds(2),
                              [&] { return s.stop.load() || !s.wal_buffer.empty(); });
            if (s.wal_buffer.empty() && s.stop.load()) {
                break;
            }
            batch.swap(s.wal_buffer);
            s.wal_buffer.clear();
            upto = s.wal_appended;
        }
        if (!batch.empty()) {
            double const t = now_us();
            if (write(s.wal_fd, batch.data(), batch.size()) < 0 || fdatasync(s.wal_fd) != 0) {
                std::perror("wal");
            }
            double const took = (now_us() - t) / 1000.0;
            if (took > 5.0) {
                char line[120];
                std::snprintf(line, sizeof line,
                              "%10.1f ms  wal: write+fdatasync of one batch took %.1f ms",
                              t / 1000.0, took);
                bg_log(s, line);
            }
            batch.clear();
        }
        {
            std::lock_guard lock(s.wal_mu);
            s.wal_durable = upto;
        }
        s.wal_cv.notify_all();
    }
    record_thread("wal-writer", start);
}

void snapshotter(Service& s, std::string const& mode, std::string const& dir)
{
    double const start = now_us();
    std::string text;
    int n = 0;
    while (!s.stop.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        if (s.stop.load()) {
            break;
        }
        double const t_begin = now_us();
        double held = 0.0;
        text.clear();
        std::size_t const chunk = mode == "whole" ? kKeys : 1000;
        for (std::size_t first = 0; first < kKeys; first += chunk) {
            double h0 = 0.0;
            double h1 = 0.0;
            {
                std::lock_guard lock(s.store_mu);
                h0 = now_us();
                char line[96];
                for (std::size_t k = first; k < first + chunk && k < kKeys; ++k) {
                    int const len =
                        std::snprintf(line, sizeof line, "%zu=%.20s\n", k, s.store[k].data());
                    text.append(line, static_cast<std::size_t>(len));
                }
                h1 = now_us();
            }
            held += h1 - h0;
            {
                std::lock_guard log_lock(s.log_mu);
                s.snapshot_holds.emplace_back(h0, h1);
            }
            if (mode == "paced") {
                std::this_thread::sleep_for(std::chrono::microseconds(200));
            }
        }
        std::string const path = dir + "/snapshot." + std::to_string(n++ % 2);
        int const fd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd >= 0) {
            if (write(fd, text.data(), text.size()) < 0) {
                std::perror("snapshot");
            }
            close(fd);
        }
        char line[120];
        std::snprintf(line, sizeof line,
                      "%10.1f ms  snapshot: %zu entries written; store lock held %.2f ms",
                      t_begin / 1000.0, kKeys, held / 1000.0);
        bg_log(s, line);
    }
    record_thread("snapshot", start);
}

void metrics_task(Service& s)
{
    double const start = now_us();
    std::mutex metrics_mu;  // its own lock, not the store lock
    while (!s.stop.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        double const t = now_us();
        {
            std::lock_guard lock(metrics_mu);
            std::uint64_t h = cpu_work(7, 400000);  // aggregate counters: a few ms of CPU
            if (h == 1) {
                std::printf("(unlikely)\n");
            }
        }
        char line[120];
        std::snprintf(line, sizeof line, "%10.1f ms  metrics: counters flushed in %.2f ms",
                      t / 1000.0, (now_us() - t) / 1000.0);
        bg_log(s, line);
    }
    record_thread("metrics", start);
}

double pct(std::vector<double> v, double p)
{
    if (v.empty()) {
        return 0.0;
    }
    auto rank = static_cast<std::size_t>(std::ceil(p / 100.0 * static_cast<double>(v.size())));
    rank = std::max<std::size_t>(rank, 1);
    std::nth_element(v.begin(), v.begin() + static_cast<long>(rank - 1), v.end());
    return v[rank - 1];
}

int main(int argc, char** argv)
{
    std::string mode = "whole";
    double seconds = 6.0;
    double rate = 2000.0;
    std::string dir = ".";
    std::string key_path;
    for (int i = 1; i + 1 < argc; i += 2) {
        std::string const a = argv[i];
        if (a == "--mode") {
            mode = argv[i + 1];
        }
        if (a == "--seconds") {
            seconds = std::stod(argv[i + 1]);
        }
        if (a == "--rate") {
            rate = std::stod(argv[i + 1]);
        }
        if (a == "--dir") {
            dir = argv[i + 1];
        }
        if (a == "--key") {
            key_path = argv[i + 1];
        }
    }
    g_t0 = Clock::now();
    Service s;
    for (std::size_t k = 0; k < kKeys; ++k) {
        std::snprintf(s.store[k].data(), s.store[k].size(), "initial-%zu", k);
    }
    s.wal_fd = open((dir + "/wal.log").c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (s.wal_fd < 0) {
        std::perror("open wal");
        return 1;
    }

    // Requests are generated in advance (fixed seed) and released at their intended times.
    Rng rng{14};
    std::vector<Request> reqs;
    for (double t = 0.0; t < seconds * 1e6; t += rng.exponential(1e6 / rate)) {
        Request r;
        r.id = static_cast<int>(reqs.size());
        r.put = rng.uniform() < 0.10;
        r.key = static_cast<std::uint32_t>(rng.next() % kKeys);
        r.intended = 50000.0 + t;  // start 50 ms after t0
        reqs.push_back(r);
    }

    std::vector<std::thread> threads;
    for (int w = 0; w < 3; ++w) {
        threads.emplace_back(worker, std::ref(s), "worker-" + std::to_string(w));
    }
    threads.emplace_back(wal_writer, std::ref(s));
    threads.emplace_back(snapshotter, std::ref(s), mode, dir);
    threads.emplace_back(metrics_task, std::ref(s));

    double const gen_start = now_us();
    for (Request& r : reqs) {
        auto const when = g_t0 + std::chrono::microseconds(static_cast<long>(r.intended));
        std::this_thread::sleep_until(when - std::chrono::microseconds(100));
        while (Clock::now() < when) {
        }
        {
            std::lock_guard lock(s.q_mu);
            s.queue.push_back(&r);
        }
        s.q_cv.notify_one();
    }
    record_thread("generator", gen_start);
    {
        std::lock_guard lock(s.q_mu);
        s.closing = true;
    }
    s.q_cv.notify_all();
    for (int w = 0; w < 3; ++w) {
        threads[static_cast<std::size_t>(w)].join();
    }
    s.stop = true;
    s.wal_cv.notify_all();
    for (std::size_t i = 3; i < threads.size(); ++i) {
        threads[i].join();
    }
    close(s.wal_fd);

    // ---------------- evidence report ----------------
    std::printf(
        "== 1. run: mode %s, offered %.0f req/s (open loop), %.0f s, %zu requests, 3 workers\n\n",
        mode.c_str(), rate, seconds, reqs.size());
    std::vector<double> get_lat;
    std::vector<double> put_lat;
    std::vector<double> all_lat;
    for (Request const& r : reqs) {
        double const l = (r.done - r.intended) / 1000.0;
        (r.put ? put_lat : get_lat).push_back(l);
        all_lat.push_back(l);
    }
    std::printf("== 2. latency at the service entry, from intended arrival to reply (ms)\n");
    std::printf("%-4s %7s %8s %8s %8s %8s %8s %8s\n", "op", "n", "mean", "p50", "p90", "p99",
                "p99.9", "max");
    for (auto const& [name, v] : {std::pair<char const*, std::vector<double>*>{"get", &get_lat},
                                  {"put", &put_lat},
                                  {"all", &all_lat}}) {
        double sum = 0;
        for (double x : *v) {
            sum += x;
        }
        std::printf("%-4s %7zu %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f\n", name, v->size(),
                    sum / static_cast<double>(v->size()), pct(*v, 50), pct(*v, 90), pct(*v, 99),
                    pct(*v, 99.9), *std::max_element(v->begin(), v->end()));
    }
    std::printf("histogram (all requests):\n");
    double edge = 0.0625;
    for (int b = 0; b < 12; ++b, edge *= 2) {
        auto const c = std::count_if(all_lat.begin(), all_lat.end(), [&](double x) {
            return x >= (b == 0 ? 0.0 : edge / 2) && x < edge;
        });
        double const share = static_cast<double>(c) / static_cast<double>(all_lat.size());
        std::string const bar(static_cast<std::size_t>(std::ceil(60.0 * share)), '#');
        std::printf("  %8.4f .. %8.4f ms %7ld %s\n", b == 0 ? 0.0 : edge / 2, edge, c, bar.c_str());
    }
    auto const over =
        std::count_if(all_lat.begin(), all_lat.end(), [&](double x) { return x >= edge / 2; });
    std::printf("  %8.4f ..      max ms %7ld\n\n", edge / 2, over);

    std::vector<Request const*> slow;
    for (Request const& r : reqs) {
        slow.push_back(&r);
    }
    std::sort(slow.begin(), slow.end(),
              [](auto* a, auto* b) { return a->done - a->intended > b->done - b->intended; });
    std::printf("== 3. traces of the 8 slowest requests (span durations in ms)\n");
    std::printf("%6s %3s %11s | %7s %7s %9s %7s %8s %7s | %7s\n", "id", "op", "arrived ms", "queue",
                "parse", "lock wait", "store", "wal wait", "reply", "total");
    for (std::size_t i = 0; i < 8 && i < slow.size(); ++i) {
        Request const& r = *slow[i];
        std::printf("%6d %3s %11.1f | %7.3f %7.3f %9.3f %7.3f %8.3f %7.3f | %7.3f\n", r.id,
                    r.put ? "put" : "get", r.intended / 1000.0, (r.dequeued - r.intended) / 1000.0,
                    (r.parsed - r.dequeued) / 1000.0, (r.lock_got - r.parsed) / 1000.0,
                    (r.lock_rel - r.lock_got) / 1000.0, (r.durable - r.lock_rel) / 1000.0,
                    (r.done - r.durable) / 1000.0, (r.done - r.intended) / 1000.0);
    }
    std::printf("\n");

    std::printf("== 4. background-task log (times since start)\n");
    std::sort(s.bg_log.begin(), s.bg_log.end());
    for (std::string const& line : s.bg_log) {
        std::printf("%s\n", line.c_str());
    }
    std::printf("\n");

    std::printf("== 5. threads: wall time, CPU time, context switches\n");
    std::printf("%-10s %9s %9s %6s %10s %10s\n", "thread", "wall ms", "cpu ms", "cpu %",
                "voluntary", "involuntary");
    std::sort(g_stats.begin(), g_stats.end(),
              [](auto const& a, auto const& b) { return a.name < b.name; });
    for (ThreadStat const& t : g_stats) {
        std::printf("%-10s %9.0f %9.1f %5.1f%% %10ld %10ld\n", t.name.c_str(), t.wall_ms, t.cpu_ms,
                    100.0 * t.cpu_ms / t.wall_ms, t.vol_cs, t.invol_cs);
    }

    // ---------------- analysis (the profile that explains the tail) ----------------
    if (!key_path.empty()) {
        std::FILE* k = std::fopen(key_path.c_str(), "w");
        if (k == nullptr) {
            return 1;
        }
        std::size_t const top = std::max<std::size_t>(1, slow.size() / 100);
        auto breakdown = [&](char const* label, std::size_t from, std::size_t count) {
            double q = 0, pa = 0, lw = 0, st = 0, wal = 0, rep = 0;
            for (std::size_t i = from; i < from + count; ++i) {
                Request const& r = *slow[i];
                q += r.dequeued - r.intended;
                pa += r.parsed - r.dequeued;
                lw += r.lock_got - r.parsed;
                st += r.lock_rel - r.lock_got;
                wal += r.durable - r.lock_rel;
                rep += r.done - r.durable;
            }
            double const tot = q + pa + lw + st + wal + rep;
            std::fprintf(
                k, "%-13s %6zu | %6.1f%% %6.1f%% %9.1f%% %6.1f%% %8.1f%% %6.1f%% | %8.3f ms\n",
                label, count, 100 * q / tot, 100 * pa / tot, 100 * lw / tot, 100 * st / tot,
                100 * wal / tot, 100 * rep / tot, tot / static_cast<double>(count) / 1000.0);
        };
        std::fprintf(k, "== A. where the time goes: share of total time in each span\n");
        std::fprintf(k, "%-13s %6s | %7s %7s %10s %7s %9s %7s | %11s\n", "requests", "n", "queue",
                     "parse", "lock wait", "store", "wal wait", "reply", "mean total");
        breakdown("all", 0, slow.size());
        breakdown("slowest 1 %", 0, top);
        breakdown("fastest 99 %", top, slow.size() - top);

        // A request "met" a snapshot hold if the hold overlapped the request's life
        // (from intended arrival to reply): it waited for the lock itself, or it waited in the
        // queue because every worker was blocked on the lock.
        auto met_hold = [&](Request const& r) {
            for (auto const& [h0, h1] : s.snapshot_holds) {
                if (h0 < r.done && h1 > r.intended) {
                    return true;
                }
            }
            return false;
        };
        std::size_t overlap_slow = 0;
        std::size_t overlap_all = 0;
        for (std::size_t i = 0; i < slow.size(); ++i) {
            if (met_hold(*slow[i])) {
                ++overlap_all;
                overlap_slow += i < top ? 1 : 0;
            }
        }
        double held = 0.0;
        double longest = 0.0;
        for (auto const& [h0, h1] : s.snapshot_holds) {
            held += h1 - h0;
            longest = std::max(longest, h1 - h0);
        }
        double const run_us = reqs.back().done - reqs.front().intended;
        std::fprintf(k, "\n== B. correlation with the snapshot's store-lock holds\n");
        std::fprintf(k, "slowest 1 %%: %zu requests; %zu of them overlap a snapshot lock hold\n",
                     top, overlap_slow);
        std::fprintf(k, "all requests: %zu; %zu of them overlap a snapshot lock hold\n",
                     slow.size(), overlap_all);
        std::fprintf(k,
                     "snapshot lock holds: %zu, longest %.2f ms, store lock held by snapshot %.2f "
                     "%% of the run\n",
                     s.snapshot_holds.size(), longest / 1000.0, 100.0 * held / run_us);
        std::fclose(k);
    }
    return 0;
}
