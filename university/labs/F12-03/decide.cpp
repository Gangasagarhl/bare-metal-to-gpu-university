// decide.cpp - SE301 F12-03: a weighted decision matrix with a sensitivity check.
// Input (stdin):
//   option <name>                                   one line per alternative
//   criterion <name> <weight> <score per option...> scores 1 (bad) .. 5 (good)
// Output: weighted totals, the winner, dominated options, and for every criterion the
// smallest change of its weight that would make a different option win.
// The numbers are JUDGEMENTS written by people; the program only makes them explicit.
#include <cmath>
#include <cstdio>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

struct Criterion
{
    std::string name;
    double weight = 0;
    std::vector<double> score;              // one per option
};

int main()
{
    std::vector<std::string> options;
    std::vector<Criterion> crit;
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream in(line);
        std::string kind;
        if (!(in >> kind) || kind[0] == '#') continue;
        if (kind == "option") {
            std::string name;
            in >> name;
            options.push_back(name);
        } else if (kind == "criterion") {
            Criterion c;
            in >> c.name >> c.weight;
            double s = 0;
            while (in >> s) c.score.push_back(s);
            if (c.score.size() != options.size()) {
                std::printf("criterion %s: %zu scores for %zu options\n", c.name.c_str(),
                            c.score.size(), options.size());
                return 2;
            }
            crit.push_back(c);
        }
    }
    const std::size_t n = options.size();
    std::vector<double> total(n, 0.0);
    double weightSum = 0;
    for (const auto& c : crit) {
        weightSum += c.weight;
        for (std::size_t o = 0; o < n; ++o) total[o] += c.weight * c.score[o];
    }
    std::size_t win = 0;
    for (std::size_t o = 1; o < n; ++o) {
        if (total[o] > total[win]) win = o;
    }
    std::printf("%-14s", "criterion");
    std::printf(" %6s", "weight");
    for (const auto& o : options) std::printf(" %10s", o.c_str());
    std::printf("\n");
    for (const auto& c : crit) {
        std::printf("%-14s %6.1f", c.name.c_str(), c.weight);
        for (double s : c.score) std::printf(" %10.0f", s);
        std::printf("\n");
    }
    std::printf("%-14s %6.1f", "TOTAL", weightSum);
    for (double t : total) std::printf(" %10.1f", t);
    std::printf("\nwinner: %s", options[win].c_str());
    double second = -std::numeric_limits<double>::infinity();
    for (std::size_t o = 0; o < n; ++o) {
        if (o != win && total[o] > second) second = total[o];
    }
    std::printf(" (margin %.1f points, %.1f %% of the winner's total)\n", total[win] - second,
                100.0 * (total[win] - second) / total[win]);

    for (std::size_t a = 0; a < n; ++a) {                       // dominated options
        for (std::size_t b = 0; b < n; ++b) {
            if (a == b) continue;
            bool noBetter = true;
            bool someWorse = false;
            for (const auto& c : crit) {
                if (c.score[a] > c.score[b]) noBetter = false;
                if (c.score[a] < c.score[b]) someWorse = true;
            }
            if (noBetter && someWorse) {
                std::printf("dominated: %s is nowhere better than %s\n", options[a].c_str(),
                            options[b].c_str());
            }
        }
    }

    // Changing weight w_c by d changes (total[win] - total[o]) by d * (s_win - s_o).
    // The winner changes when that difference reaches 0 (weights must stay >= 0).
    std::printf("sensitivity: weight change that makes another option win\n");
    for (const auto& c : crit) {
        double best = std::numeric_limits<double>::infinity();
        std::string by;
        for (std::size_t o = 0; o < n; ++o) {
            if (o == win) continue;
            const double gap = total[win] - total[o];
            const double slope = c.score[win] - c.score[o];
            if (slope == 0) continue;
            const double d = -gap / slope;
            if (c.weight + d < 0) continue;
            if (std::fabs(d) < std::fabs(best)) {
                best = d;
                by = options[o];
            }
        }
        if (std::isinf(best)) {
            std::printf("  %-14s no weight change flips the winner\n", c.name.c_str());
        } else {
            std::printf("  %-14s %+6.2f (from %.1f to %.2f) ties with %s; beyond it, %s wins\n",
                        c.name.c_str(), best, c.weight, c.weight + best, by.c_str(), by.c_str());
        }
    }
    return 0;
}
