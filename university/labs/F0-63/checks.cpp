// checks.cpp - recomputes the numbers used in the text of chapter F0-63.
#include "benchstats.hpp"

#include <cmath>
#include <iostream>
#include <vector>

int rank(double p, int n)
{
    return static_cast<int>(std::ceil(p / 100.0 * n));
}

int main()
{
    std::cout << "Worked example (exercise data, ms):\n";
    std::vector<double> t = {12.1, 11.8, 12.0, 25.3, 11.9, 12.2, 12.0, 11.7, 12.4, 30.6};
    benchstats::Summary s = benchstats::summarise(t);
    benchstats::print(std::cout, s, "ms");
    std::cout << "sum " << s.mean * 10 << "; mean of the 8 values without 25.3 and 30.6: "
              << (s.mean * 10 - 25.3 - 30.6) / 8 << "\n";
    std::cout << "\nNearest ranks: p25 of 10 -> " << rank(25, 10) << ", p75 of 10 -> "
              << rank(75, 10) << ", p90 of 10 -> " << rank(90, 10) << ", p90 of 20 -> "
              << rank(90, 20) << ", p99 of 30 -> " << rank(99, 30) << ", p99 of 100 -> "
              << rank(99, 100) << ", p99 of 1000 -> " << rank(99, 1000) << "\n";
    std::cout << "\nCheck yourself 1: 8 9 7 30 8 -> ";
    benchstats::Summary q1 = benchstats::summarise({8, 9, 7, 30, 8});
    std::cout << "median " << q1.median << ", mean " << q1.mean << "\n";
    std::cout << "Check yourself 4: 1..10 -> ";
    benchstats::Summary q4 = benchstats::summarise({1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
    std::cout << "q1 " << q4.q1 << ", q3 " << q4.q3 << ", IQR " << q4.iqr << ", fences ["
              << q4.lowFence << ", " << q4.highFence << "]\n";
    std::cout << "Check yourself 5: ratio 9.7/10.0 = " << 9.7 / 10.0
              << "; middle halves [9.6, 10.5] and [9.4, 10.2] overlap\n";
    std::cout << "\nForensic: claim 17.511 / 2.640 = " << 17.511 / 2.640 << "\n";
    return 0;
}
