// claim_check.cpp - forensic analysis for "The benchmark that lied" (chapter F0-63).
// Reads the frozen capture (claim_check.in): lines "run A_ms B_ms"; lines starting with '#' skipped.
#include "benchstats.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

int main()
{
    std::vector<double> a;
    std::vector<double> b;
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        std::istringstream fields(line);
        int run = 0;
        double ta = 0.0;
        double tb = 0.0;
        if (fields >> run >> ta >> tb) {
            a.push_back(ta);
            b.push_back(tb);
        }
    }
    if (a.size() < 5) {
        std::cout << "need at least 5 runs of each version\n";
        return 1;
    }
    std::cout << "runs read: " << a.size() << " of A, " << b.size() << " of B\n";
    std::cout << "THE CLAIM (one run each): A " << a[0] << " ms, B " << b[0]
              << " ms -> 'B is " << a[0] / b[0] << " times faster'\n";
    std::cout << "first five pairs (ms):";
    for (std::size_t i = 0; i < 5; ++i) {
        std::cout << "  " << a[i] << "/" << b[i];
    }
    std::cout << "\n\nALL 30 RUNS\nA: ";
    benchstats::Summary sa = benchstats::summarise(a);
    benchstats::print(std::cout, sa, "ms");
    std::cout << "B: ";
    benchstats::Summary sb = benchstats::summarise(b);
    benchstats::print(std::cout, sb, "ms");
    benchstats::Comparison c = benchstats::compare(sa, sb);
    std::cout << "median B / median A = " << c.ratio
              << (c.overlap ? " ; middle halves OVERLAP" : " ; middle halves separate") << "\n";
    std::cout << "means: A " << sa.mean << ", B " << sb.mean << " (pulled up by run 1)\n";
    std::cout << "\nRUNS 2-30 ONLY (run 1 treated as warm-up)\n";
    std::vector<double> a2(a.begin() + 1, a.end());
    std::vector<double> b2(b.begin() + 1, b.end());
    benchstats::Summary sa2 = benchstats::summarise(a2);
    benchstats::Summary sb2 = benchstats::summarise(b2);
    std::cout << "A: ";
    benchstats::print(std::cout, sa2, "ms");
    std::cout << "B: ";
    benchstats::print(std::cout, sb2, "ms");
    std::cout << "median B / median A = " << benchstats::compare(sa2, sb2).ratio << "\n";
    return 0;
}
