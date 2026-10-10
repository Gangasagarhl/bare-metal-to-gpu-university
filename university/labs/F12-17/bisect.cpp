// bisect.cpp - F12-17 Listing 2: bisecting a latency regression with noisy measurements.
// 64 builds; build 41 makes the true p99 latency go from 20 ms to 23 ms. Each measurement
// is the true value times (1 + noise), noise uniform in [-10 %, +10 %]. A build is called
// "bad" if its measurement (or the median of k measurements) is above 21.5 ms.
// 1000 trials per method; each trial uses its own seed.
#include <algorithm>
#include <cstdio>
#include <random>
#include <vector>

const int kBuilds = 64;
const int kCulprit = 41;

double measure(int build, std::mt19937& rng)
{
    const double truth = build >= kCulprit ? 23.0 : 20.0;
    const double noise = (static_cast<int>(rng() % 2001) - 1000) / 10000.0;  // -0.1 .. +0.1
    return truth * (1.0 + noise);
}

bool looks_bad(int build, int k, std::mt19937& rng, int& count, bool show)
{
    std::vector<double> m;
    for (int i = 0; i < k; ++i) {
        m.push_back(measure(build, rng));
        ++count;
    }
    std::sort(m.begin(), m.end());
    const double med = m[m.size() / 2];
    if (show) {
        std::printf("    build %2d: median of %d = %5.2f ms -> %s\n", build, k, med,
                    med > 21.5 ? "bad" : "good");
    }
    return med > 21.5;
}

// Classic bisection: "good" is known good, "bad" is known bad; returns the first bad build.
int bisect(int k, std::mt19937& rng, int& count, bool show)
{
    int good = 0, bad = kBuilds - 1;
    while (bad - good > 1) {
        const int mid = (good + bad) / 2;
        if (looks_bad(mid, k, rng, count, show)) {
            bad = mid;
        } else {
            good = mid;
        }
    }
    return bad;
}

int main()
{
    std::printf("true first bad build: %d\n\n", kCulprit);
    int shown = -1;
    for (const int k : {1, 3, 7}) {
        int correct = 0;
        long measurements = 0;
        for (int trial = 0; trial < 1000; ++trial) {
            std::mt19937 rng(static_cast<unsigned>(trial));
            int count = 0;
            if (bisect(k, rng, count, false) == kCulprit) {
                ++correct;
            } else if (k == 1 && shown < 0) {
                shown = trial;
            }
            measurements += count;
        }
        std::printf("k = %d measurement(s) per step: right build in %4d of 1000 trials, "
                    "%.1f measurements per bisection\n", k, correct, measurements / 1000.0);
    }
    for (const int k : {1, 7}) {
        std::printf("\ntrial %d replayed with k = %d:\n", shown, k);
        std::mt19937 rng(static_cast<unsigned>(shown));
        int count = 0;
        const int found = bisect(k, rng, count, true);
        std::printf("  result: build %d (%s)\n", found, found == kCulprit ? "right" : "WRONG");
    }
    return 0;
}
