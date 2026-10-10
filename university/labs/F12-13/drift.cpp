// drift.cpp - F12-13 forensic evidence generator: "the 30 % speed-up that vanished".
// Exercise simulation with a fixed seed (not a measurement). Variant B is truly 2 % faster
// than A. In the first benchmark, all A runs happened while another job slowed the machine
// down by a factor of 1.38; all B runs happened after that job ended.
#include "abstat.hpp"

#include <cstdint>
#include <cstdio>
#include <vector>

struct Noise
{
    abstat::Rng rng{99};
    double factor()  // multiplicative run-to-run noise, uniform in [0.97, 1.03]
    {
        return 0.97 + 0.06 * static_cast<double>(rng.next() >> 11) * 0x1.0p-53;
    }
};

void clock_text(int seconds_after_midnight, char* out)
{
    std::snprintf(out, 16, "%02d:%02d:%02d", seconds_after_midnight / 3600,
                  seconds_after_midnight / 60 % 60, seconds_after_midnight % 60);
}

int main()
{
    double const a_true = 10.0;
    double const b_true = 9.8;
    int const job_end = 2 * 3600 + 2 * 60 + 50;  // 02:02:50
    Noise noise;
    char t[16];

    std::printf("== evidence 1: benchmark log attached to pull request 4127 (night run)\n");
    std::vector<double> a;
    std::vector<double> b;
    int now = 1 * 3600 + 58 * 60;  // 01:58:00
    for (int i = 0; i < 40; ++i, now += 15) {
        bool const is_a = i < 20;
        double const slow = now < job_end ? 1.38 : 1.0;
        double const ms = (is_a ? a_true : b_true) * slow * noise.factor();
        (is_a ? a : b).push_back(ms);
        clock_text(now, t);
        std::printf("%s run %2d variant %s %6.2f ms\n", t, i % 20 + 1, is_a ? "A" : "B", ms);
    }
    abstat::Interval const first = abstat::ratio_ci(a, b);
    std::printf("summary in the PR: median A %.2f ms, median B %.2f ms, B is %.0f %% faster;\n",
                abstat::median(a), abstat::median(b), 100.0 * (1.0 - first.ratio));
    std::printf("  ratio B/A %.3f, 95 %% bootstrap interval [%.3f, %.3f]\n\n", first.ratio,
                first.low, first.high);

    std::printf("== evidence 2: the same benchmark re-run by CI the next afternoon, interleaved\n");
    std::vector<double> a2;
    std::vector<double> b2;
    for (int r = 0; r < 21; ++r) {
        a2.push_back(a_true * noise.factor());
        b2.push_back(b_true * noise.factor());
    }
    abstat::Interval const second = abstat::ratio_ci(a2, b2);
    std::printf("21 rounds, order A B / B A alternating: median A %.2f ms, median B %.2f ms\n",
                abstat::median(a2), abstat::median(b2));
    std::printf("  ratio B/A %.3f, 95 %% bootstrap interval [%.3f, %.3f]\n\n", second.ratio,
                second.low, second.high);

    std::printf("== evidence 3: the benchmark machine's job scheduler log, that night\n");
    std::printf(
        "01:30:00 scheduler: job nightly-backup started (compress and upload, 4 threads)\n");
    std::printf("01:45:00 scheduler: job metrics-export started\n");
    std::printf("01:45:04 scheduler: job metrics-export finished\n");
    std::printf("01:57:40 ci-runner: job bench-pr-4127 started\n");
    clock_text(job_end, t);
    std::printf("%s scheduler: job nightly-backup finished\n", t);
    std::printf("02:05:00 scheduler: job log-rotate started\n");
    std::printf("02:05:01 scheduler: job log-rotate finished\n");
    std::printf("02:08:05 ci-runner: job bench-pr-4127 finished\n");
    return 0;
}
