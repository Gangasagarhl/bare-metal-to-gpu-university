// summary.cpp - order statistics of a list of timings (one number per line on stdin).
#include "benchstats.hpp"

#include <algorithm>
#include <iostream>
#include <vector>

int main()
{
    std::vector<double> t;
    double x = 0.0;
    while (std::cin >> x) {
        t.push_back(x);
    }
    std::vector<double> sorted = t;
    std::sort(sorted.begin(), sorted.end());
    std::cout << "rank  value\n";
    for (std::size_t i = 0; i < sorted.size(); ++i) {
        std::cout << i + 1 << "     " << sorted[i] << "\n";
    }
    benchstats::Summary s = benchstats::summarise(t);
    benchstats::print(std::cout, s, "ms");
    std::cout << "flagged as outliers (outside the fences):";
    for (double v : t) {
        if (v < s.lowFence || v > s.highFence) {
            std::cout << " " << v;
        }
    }
    std::cout << "\nmedian without the outliers' influence: " << s.median
              << " ms; mean with them: " << s.mean << " ms\n";
    return 0;
}
