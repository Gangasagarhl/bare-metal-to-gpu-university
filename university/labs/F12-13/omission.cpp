// omission.cpp - F12-13 Listing 1: coordinated omission, shown with a deterministic simulation.
// A server answers every request in exactly 1 ms, except that it stalls completely from
// t = 30 s to t = 32 s (think of a 2-second pause). A load generator intends to send one
// request every 10 ms for 60 s (100 req/s). Three ways to measure the same 60 seconds:
//   A  closed loop, latency from the ACTUAL send: the generator waits for each answer before
//      sending the next one, so during the stall it sends nothing and records one slow sample;
//   B  the same closed-loop run, latency from the INTENDED send time (the "corrected" view);
//   C  open loop: requests are sent at the intended times whether or not earlier ones returned.
// All times are simulated milliseconds; no real system is measured.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

double const kStallStart = 30000.0;
double const kStallEnd = 32000.0;
double const kService = 1.0;

// Finish time of a request whose service starts at 'start' (the stall freezes the server).
double finish(double start)
{
    if (start >= kStallStart && start < kStallEnd) {
        start = kStallEnd;
    }
    double done = start + kService;
    if (start < kStallStart && done > kStallStart) {
        done += kStallEnd - kStallStart;
    }
    return done;
}

double pct(std::vector<double> v, double p)
{
    auto rank = static_cast<std::size_t>(std::ceil(p / 100.0 * static_cast<double>(v.size())));
    std::nth_element(v.begin(), v.begin() + static_cast<long>(rank - 1), v.end());
    return v[rank - 1];
}

void report(char const* name, std::vector<double> const& v)
{
    double sum = 0.0;
    for (double x : v) {
        sum += x;
    }
    std::printf("%-44s %6zu %8.2f %8.2f %9.2f %9.2f %9.1f\n", name, v.size(),
                sum / static_cast<double>(v.size()), pct(v, 50.0), pct(v, 99.0), pct(v, 99.9),
                *std::max_element(v.begin(), v.end()));
}

int main()
{
    double const gap = 10.0;         // intended: one request every 10 ms
    double const horizon = 60000.0;  // 60 s

    // A and B: one closed-loop connection. Request i is due at i * gap, but it cannot be sent
    // before the answer to request i - 1 has arrived.
    std::vector<double> from_actual;
    std::vector<double> from_intended;
    double answer = 0.0;
    for (double due = 0.0; due < horizon; due += gap) {
        double const sent = std::max(due, answer);
        answer = finish(sent);
        from_actual.push_back(answer - sent);
        from_intended.push_back(answer - due);
    }

    // C: open loop, one request every 10 ms regardless; the server serves them in order.
    std::vector<double> open;
    double server_free = 0.0;
    for (double due = 0.0; due < horizon; due += gap) {
        double const done = finish(std::max(due, server_free));
        server_free = done;
        open.push_back(done - due);
    }

    std::printf("server: 1 ms per request, frozen from 30.0 s to 32.0 s; run length 60 s\n");
    std::printf("generator intends one request every 10 ms (100 req/s)\n\n");
    std::printf("%-44s %6s %8s %8s %9s %9s %9s\n", "measurement", "n", "mean ms", "p50 ms",
                "p99 ms", "p99.9 ms", "max ms");
    report("A closed loop, timed from actual send", from_actual);
    report("B closed loop, timed from intended send", from_intended);
    report("C open loop, timed from intended send", open);

    auto slow = [](std::vector<double> const& v) {
        return std::count_if(v.begin(), v.end(), [](double x) { return x > 100.0; });
    };
    std::printf("\nrequests slower than 100 ms: A %ld, B %ld, C %ld\n", slow(from_actual),
                slow(from_intended), slow(open));
    std::printf("A sent the requests due during the stall late, back to back after it ended,\n");
    std::printf("and timed each one from its late send: the waiting was never recorded.\n");
    return 0;
}
