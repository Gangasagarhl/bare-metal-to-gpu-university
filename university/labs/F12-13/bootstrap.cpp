// bootstrap.cpp - F12-13 Listing 2: is variant B really faster than the baseline?
// Reads lines "name t1 t2 ... tn" (run times in ms); the first line is the baseline.
// For every other line it prints the medians, the mean, the ratio median(B)/median(baseline)
// and its 95 % bootstrap interval (abstat.hpp), and a verdict that the interval supports.
#include "abstat.hpp"

#include <algorithm>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

struct Variant
{
    std::string name;
    std::vector<double> runs;
};

double mean(std::vector<double> const& v)
{
    double sum = 0.0;
    for (double x : v) {
        sum += x;
    }
    return sum / static_cast<double>(v.size());
}

int main()
{
    std::vector<Variant> variants;
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        std::istringstream in(line);
        Variant v;
        in >> v.name;
        for (double t; in >> t;) {
            v.runs.push_back(t);
        }
        variants.push_back(v);
    }
    Variant const& base = variants.front();
    std::printf("%-7s runs %zu  median %.2f ms  mean %.2f ms  min %.2f  max %.2f\n\n",
                base.name.c_str(), base.runs.size(), abstat::median(base.runs), mean(base.runs),
                *std::min_element(base.runs.begin(), base.runs.end()),
                *std::max_element(base.runs.begin(), base.runs.end()));
    for (std::size_t i = 1; i < variants.size(); ++i) {
        Variant const& v = variants[i];
        abstat::Interval const ci = abstat::ratio_ci(base.runs, v.runs);
        std::printf("%-7s runs %zu  median %.2f ms  mean %.2f ms\n", v.name.c_str(), v.runs.size(),
                    abstat::median(v.runs), mean(v.runs));
        std::printf(
            "        ratio of medians %.3f  (%+.1f %%)  95 %% bootstrap interval [%.3f, %.3f]\n",
            ci.ratio, 100.0 * (ci.ratio - 1.0), ci.low, ci.high);
        char const* verdict = ci.high < 1.0  ? "faster: the whole interval is below 1"
                              : ci.low > 1.0 ? "slower: the whole interval is above 1"
                                             : "no difference shown: the interval contains 1";
        std::printf("        verdict: %s\n\n", verdict);
    }
    return 0;
}
