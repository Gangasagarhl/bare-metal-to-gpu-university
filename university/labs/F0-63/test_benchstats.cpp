// test_benchstats.cpp - tests for the MA202 project header benchstats.hpp.
// Every expected value below was computed by hand in chapter F0-63.
#include "benchstats.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

int failures = 0;

void expect(bool ok, char const* what)
{
    std::cout << (ok ? "pass  " : "FAIL  ") << what << "\n";
    failures += ok ? 0 : 1;
}

bool near(double a, double b)
{
    return std::fabs(a - b) < 1e-9;
}

int main()
{
    using benchstats::summarise;
    // The worked example: 10 timings in ms (exercise data).
    std::vector<double> t = {12.1, 11.8, 12.0, 25.3, 11.9, 12.2, 12.0, 11.7, 12.4, 30.6};
    benchstats::Summary s = summarise(t);
    expect(s.n == 10, "n = 10");
    expect(near(s.median, 12.05), "median of even n = mean of the two middle values (12.05)");
    expect(near(s.q1, 11.9), "q1 = rank ceil(2.5) = 3rd value (11.9)");
    expect(near(s.q3, 12.4), "q3 = rank ceil(7.5) = 8th value (12.4)");
    expect(near(s.p90, 25.3), "p90 = rank 9 (25.3)");
    expect(near(s.mean, 15.2), "mean 15.2");
    expect(s.outliersHigh == 2 && s.outliersLow == 0, "two high outliers (25.3, 30.6)");
    // Odd n and the edge ranks.
    std::vector<double> odd = {5.0, 1.0, 3.0};
    benchstats::Summary o = summarise(odd);
    expect(near(o.median, 3.0), "median of odd n = middle value");
    expect(near(o.p90, 5.0), "p90 of 3 values = rank ceil(2.7) = 3");
    expect(near(benchstats::percentile({1.0, 2.0}, 1.0), 1.0), "p1 clamps to rank 1");
    expect(near(benchstats::percentile({1.0, 2.0}, 100.0), 2.0), "p100 = maximum");
    // One value: sd 0, no outliers.
    benchstats::Summary one = summarise({7.0});
    expect(near(one.sd, 0.0) && one.outliersHigh == 0, "single value: sd 0");
    // Empty input must be refused, not divided by zero.
    bool threw = false;
    try {
        summarise({});
    } catch (std::invalid_argument const&) {
        threw = true;
    }
    expect(threw, "empty input throws std::invalid_argument");
    // Comparison: overlapping middle halves.
    benchstats::Summary a = summarise({10.0, 10.2, 10.4, 10.6, 10.8});
    benchstats::Summary b = summarise({10.1, 10.3, 10.5, 10.7, 10.9});
    benchstats::Comparison c = benchstats::compare(a, b);
    expect(c.overlap, "overlapping middle halves detected");
    benchstats::Summary f = summarise({5.0, 5.1, 5.2, 5.3, 5.4});
    expect(!benchstats::compare(a, f).overlap, "separated middle halves detected");
    expect(near(benchstats::compare(a, f).ratio, 5.2 / 10.4), "ratio of medians");
    // Timing helper returns one value per timed run, none negative.
    int calls = 0;
    std::vector<double> ms = benchstats::timeRuns([&calls] { ++calls; }, 3, 30);
    bool nonNegative = true;
    for (double v : ms) {
        nonNegative = nonNegative && v >= 0.0;
    }
    expect(ms.size() == 30 && calls == 33 && nonNegative, "timeRuns: 3 warm-ups + 30 timed");
    std::cout << failures << " failure(s)\n";
    return failures == 0 ? 0 : 1;
}
