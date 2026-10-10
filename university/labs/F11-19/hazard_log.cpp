// hazard_log.cpp - F11-19: check a hazard log with the course's own risk graph.
// Input (stdin), one record per line:
//   MODES  <mode> <mode> ...                     operating modes the robot has
//   USAGE  <mode> <percent> <mode> <percent> ... share of operating time per mode
//   H|<id>|<mode>|<hazard>|<S>|<E>|<A>|<safety functions, comma separated or ->|<tests or ->
// Course risk graph (SS402's own, NOT the table of any standard):
//   S severity 1..4, E exposure 1..3, A avoidability 1..2; points = 2*S + E + A
//   course target CT0 (points <= 7), CT1 (8-9), CT2 (10-11), CT3 (12-13)
// Checks: every mode has hazards; CT1+ hazards have a safety function; every safety function
// has a test; the declared exposure is not lower than the share of time in the mode implies.
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

struct Hazard {
    std::string id, mode, text, functions, tests;
    int s = 0, e = 0, a = 0;
};

int course_target(int s, int e, int a)
{
    const int points = 2 * s + e + a;
    if (points <= 7) {
        return 0;
    }
    return std::min(3, (points - 6) / 2);
}

int exposure_from_share(double percent)
{
    if (percent >= 10.0) {
        return 3;
    }
    return percent >= 1.0 ? 2 : 1;
}

std::vector<std::string> split(const std::string& s, char sep)
{
    std::vector<std::string> parts;
    std::string item;
    std::istringstream in(s);
    while (std::getline(in, item, sep)) {
        parts.push_back(item);
    }
    return parts;
}

int main()
{
    std::vector<std::string> modes;
    std::map<std::string, double> usage;
    std::vector<Hazard> log;
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        std::istringstream in(line);
        std::string word;
        if (line.rfind("MODES", 0) == 0) {
            in >> word;
            while (in >> word) {
                modes.push_back(word);
            }
        } else if (line.rfind("USAGE", 0) == 0) {
            in >> word;
            double pct = 0.0;
            while (in >> word >> pct) {
                usage[word] = pct;
            }
        } else {
            const auto f = split(line, '|');
            if (f.size() != 9 || f[0] != "H") {
                std::cerr << "bad line: " << line << '\n';
                return 2;
            }
            log.push_back({f[1], f[2], f[3], f[7], f[8], std::stoi(f[4]), std::stoi(f[5]),
                           std::stoi(f[6])});
        }
    }

    std::puts("Course risk graph: points = 2*S + E + A -> course target (CT)");
    std::puts("        E1A1 E1A2 E2A1 E2A2 E3A1 E3A2");
    for (int s = 1; s <= 4; ++s) {
        std::printf("  S%d   ", s);
        for (int e = 1; e <= 3; ++e) {
            for (int a = 1; a <= 2; ++a) {
                std::printf(" CT%d ", course_target(s, e, a));
            }
        }
        std::puts("");
    }

    std::stable_sort(log.begin(), log.end(), [](const Hazard& x, const Hazard& y) {
        return course_target(x.s, x.e, x.a) > course_target(y.s, y.e, y.a);
    });
    std::puts("\nHazard log, highest course target first:");
    std::puts("  id   mode        S E A  CT  safety functions   tests       hazard");
    for (const auto& h : log) {
        std::printf("  %-4s %-11s %d %d %d  CT%d %-18s %-11s %s\n", h.id.c_str(), h.mode.c_str(),
                    h.s, h.e, h.a, course_target(h.s, h.e, h.a), h.functions.c_str(),
                    h.tests.c_str(), h.text.c_str());
    }

    int findings = 0;
    std::puts("\nReview checks:");
    for (const auto& m : modes) {
        const auto n = std::count_if(log.begin(), log.end(),
                                     [&](const Hazard& h) { return h.mode == m; });
        if (n == 0) {
            std::printf("  FINDING mode '%s' has no hazards analysed\n", m.c_str());
            ++findings;
        }
    }
    for (const auto& h : log) {
        const int ct = course_target(h.s, h.e, h.a);
        if (ct >= 1 && h.functions == "-") {
            std::printf("  FINDING %s is CT%d but has no safety function\n", h.id.c_str(), ct);
            ++findings;
        }
        if (h.functions != "-" && h.tests == "-") {
            std::printf("  FINDING %s has safety functions %s but no test\n", h.id.c_str(),
                        h.functions.c_str());
            ++findings;
        }
        if (usage.count(h.mode) != 0) {
            const int implied = exposure_from_share(usage.at(h.mode));
            if (h.e < implied) {
                std::printf("  FINDING %s declares E%d but mode '%s' is %.0f %% of operating time"
                            " (E%d); with E%d it would be CT%d\n",
                            h.id.c_str(), h.e, h.mode.c_str(), usage.at(h.mode), implied,
                            implied, course_target(h.s, implied, h.a));
                ++findings;
            }
        }
    }
    if (findings == 0) {
        std::puts("  (no findings)");
    }
    std::printf("\n%zu hazards, %zu modes, %d findings: %s\n", log.size(), modes.size(), findings,
                findings == 0 ? "the log passes the course's completeness checks"
                              : "the log is NOT ready for review");
    return 0;
}
