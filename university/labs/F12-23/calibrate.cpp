// calibrate.cpp - turn your own history of estimates into a range for the next one.
// Input lines:  m <milestone> <estimated hours> <actual hours>
//               new <milestone> <estimated hours>
// For every finished milestone the ratio actual / estimate is computed; the median
// and the 80th percentile (nearest rank) of your ratios scale the new estimate.
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

double nearestRank(const std::vector<double>& sorted, double p)
{
    auto rank = static_cast<std::size_t>(std::ceil(p * static_cast<double>(sorted.size())));
    rank = std::clamp<std::size_t>(rank, 1, sorted.size());
    return sorted[rank - 1];
}

int main()
{
    std::vector<double> ratios;
    std::vector<std::pair<std::string, double>> next;
    double estSum = 0, actSum = 0;
    std::string line;
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "milestone       estimate  actual  ratio\n";
    while (std::getline(std::cin, line)) {
        std::istringstream in(line);
        std::string kind, name;
        if (!(in >> kind) || kind[0] == '#') {
            continue;
        }
        double est = 0, act = 0;
        if (kind == "m" && in >> name >> est >> act && est > 0 && act > 0) {
            ratios.push_back(act / est);
            estSum += est;
            actSum += act;
            std::cout << std::left << std::setw(14) << name << std::right << std::setw(9) << est
                      << std::setw(8) << act << std::setw(7) << act / est << '\n';
        } else if (kind == "new" && in >> name >> est && est > 0) {
            next.emplace_back(name, est);
        } else {
            std::cout << "cannot read: " << line << '\n';
            return 2;
        }
    }
    if (ratios.empty()) {
        std::cout << "no finished milestones: no history to calibrate with\n";
        return 2;
    }
    std::vector<double> sorted = ratios;
    std::sort(sorted.begin(), sorted.end());
    const double med = nearestRank(sorted, 0.5);
    const double p80 = nearestRank(sorted, 0.8);
    std::cout << "\nfinished milestones: " << ratios.size() << "\n";
    std::cout << "total estimate " << estSum << " h, total actual " << actSum
              << " h, overall ratio "
              << actSum / estSum << "\n";
    std::cout << "ratio: min " << sorted.front() << "  median " << med << "  P80 " << p80
              << "  max " << sorted.back() << "\n";
    const auto under =
        std::count_if(ratios.begin(), ratios.end(), [](double r) { return r > 1.0; });
    std::cout << "milestones that took longer than estimated: " << under << " of " << ratios.size()
              << "\n";
    if (ratios.size() < 5) {
        std::cout << "warning: fewer than 5 finished milestones; treat the range as rough\n";
    }
    for (const auto& [name, est] : next) {
        std::cout << "next " << name << ": raw estimate " << est << " h -> calibrated range "
                  << est * med << " h (median) to " << est * p80 << " h (P80)\n";
    }
    return 0;
}
