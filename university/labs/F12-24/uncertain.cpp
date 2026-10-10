// uncertain.cpp - comparing options when the future is uncertain.
// Input lines:
//   scenario <name> <probability>          (probabilities must add up to 1)
//   option   <name> <payoff in each scenario, in scenario order>
//   spike    <name> <cost> <accuracy>      (optional) an experiment that reports the
//            true scenario with probability <accuracy>, otherwise one of the other
//            scenarios, each equally likely
// Payoffs are net benefit in team-days (positive = good). Prints expected value,
// worst case, maximum regret, the value of perfect information, the value of each
// spike, and how the expected-value choice moves with the first scenario's probability.
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

struct Option
{
    std::string name;
    std::vector<double> pay;
};

struct Spike
{
    std::string name;
    double cost = 0, accuracy = 0;
};

double expectedValue(const Option& o, const std::vector<double>& p)
{
    double ev = 0;
    for (std::size_t s = 0; s < p.size(); ++s) {
        ev += p[s] * o.pay[s];
    }
    return ev;
}

// Index of the option with the largest expected value (the first one on a tie).
std::size_t bestByEv(const std::vector<Option>& opts, const std::vector<double>& p)
{
    std::size_t best = 0;
    for (std::size_t i = 1; i < opts.size(); ++i) {
        if (expectedValue(opts[i], p) > expectedValue(opts[best], p) + 1e-12) {
            best = i;
        }
    }
    return best;
}

int main()
{
    std::vector<std::string> scen;
    std::vector<double> prob;
    std::vector<Option> opts;
    std::vector<Spike> spikes;
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream in(line);
        std::string kind, name;
        if (!(in >> kind) || kind[0] == '#') {
            continue;
        }
        in >> name;
        if (kind == "scenario") {
            double p = -1;
            in >> p;
            scen.push_back(name);
            prob.push_back(p);
        } else if (kind == "option") {
            Option o{name, {}};
            double v = 0;
            while (in >> v) {
                o.pay.push_back(v);
            }
            if (o.pay.size() != scen.size()) {
                std::cout << "option " << name << ": " << o.pay.size() << " payoffs for "
                          << scen.size() << " scenarios\n";
                return 2;
            }
            opts.push_back(o);
        } else if (kind == "spike") {
            Spike k{name, 0, 0};
            in >> k.cost >> k.accuracy;
            spikes.push_back(k);
        }
    }
    double total = 0;
    for (double p : prob) {
        if (p < 0) {
            std::cout << "a scenario has a negative or missing probability\n";
            return 2;
        }
        total += p;
    }
    if (scen.empty() || opts.empty() || std::fabs(total - 1.0) > 1e-9) {
        std::cout << "probabilities add up to " << total << ", not 1: fix the scenarios first\n";
        return 2;
    }
    const std::size_t S = scen.size();

    // Best payoff in each scenario, for regret and for perfect information.
    std::vector<double> bestIn(S, -std::numeric_limits<double>::infinity());
    for (const Option& o : opts) {
        for (std::size_t s = 0; s < S; ++s) {
            bestIn[s] = std::max(bestIn[s], o.pay[s]);
        }
    }

    std::cout << std::fixed << std::setprecision(2);
    std::cout << std::left << std::setw(13) << "option";
    for (std::size_t s = 0; s < S; ++s) {
        const std::string head = scen[s] + "(" + std::to_string(prob[s]).substr(0, 4) + ")";
        std::cout << std::right << std::setw(18) << head;
    }
    std::cout << "  expected   worst  max regret\n";
    std::size_t evBest = 0, worstBest = 0, regretBest = 0;
    std::vector<double> worst(opts.size()), regret(opts.size());
    for (std::size_t i = 0; i < opts.size(); ++i) {
        const Option& o = opts[i];
        worst[i] = *std::min_element(o.pay.begin(), o.pay.end());
        regret[i] = 0;
        for (std::size_t s = 0; s < S; ++s) {
            regret[i] = std::max(regret[i], bestIn[s] - o.pay[s]);
        }
        std::cout << std::left << std::setw(13) << o.name << std::right;
        for (double v : o.pay) {
            std::cout << std::setw(18) << v;
        }
        std::cout << std::setw(10) << expectedValue(o, prob) << std::setw(8) << worst[i]
                  << std::setw(12) << regret[i] << '\n';
        if (worst[i] > worst[worstBest]) {
            worstBest = i;
        }
        if (regret[i] < regret[regretBest]) {
            regretBest = i;
        }
    }
    evBest = bestByEv(opts, prob);
    const double evNow = expectedValue(opts[evBest], prob);
    std::cout << "\nchoice by expected value:       " << opts[evBest].name << '\n';
    std::cout << "choice by best worst case:      " << opts[worstBest].name << '\n';
    std::cout << "choice by smallest max regret:  " << opts[regretBest].name << '\n';

    double perfect = 0;
    for (std::size_t s = 0; s < S; ++s) {
        perfect += prob[s] * bestIn[s];
    }
    std::cout << "\nexpected value deciding now:              " << evNow << '\n';
    std::cout << "expected value with perfect information:  " << perfect << '\n';
    std::cout << "value of perfect information (upper bound for any spike): " << perfect - evNow
              << '\n';

    for (const Spike& k : spikes) {
        // Likelihood of signal j given true scenario s.
        auto like = [&](std::size_t j, std::size_t s) {
            if (S == 1) {
                return 1.0;
            }
            return j == s ? k.accuracy : (1.0 - k.accuracy) / static_cast<double>(S - 1);
        };
        double withSpike = 0;
        std::cout << "\nspike " << k.name << " (cost " << k.cost << ", accuracy " << k.accuracy
                  << "):\n";
        for (std::size_t j = 0; j < S; ++j) {
            double pj = 0;
            for (std::size_t s = 0; s < S; ++s) {
                pj += prob[s] * like(j, s);
            }
            if (pj <= 0) {
                continue;
            }
            std::vector<double> post(S);
            for (std::size_t s = 0; s < S; ++s) {
                post[s] = prob[s] * like(j, s) / pj;
            }
            const std::size_t b = bestByEv(opts, post);
            const double ev = expectedValue(opts[b], post);
            withSpike += pj * ev;
            std::cout << "  if it says " << std::left << std::setw(12) << scen[j] << std::right
                      << " (probability " << pj << "): choose " << opts[b].name
                      << ", expected " << ev << '\n';
        }
        const double value = withSpike - evNow;
        std::cout << "  expected value with the spike, before its cost: " << withSpike << '\n';
        std::cout << "  value of the spike: " << value << ", cost " << k.cost << " -> "
                  << (value > k.cost ? "worth running first" : "not worth its cost") << '\n';
    }

    // Sensitivity: move the first scenario's probability from 0 to 1, keeping the
    // other scenarios in their original proportions.
    std::cout << "\nexpected-value choice as p(" << scen[0] << ") moves (others in proportion):\n";
    std::size_t prev = opts.size();
    double from = 0;
    std::cout << std::setprecision(3);
    for (int k = 0; k <= 1000; ++k) {
        const double p0 = k / 1000.0;
        std::vector<double> p(S);
        p[0] = p0;
        for (std::size_t s = 1; s < S; ++s) {
            p[s] = prob[0] < 1.0 ? prob[s] * (1.0 - p0) / (1.0 - prob[0])
                                 : (1.0 - p0) / static_cast<double>(S - 1);
        }
        const std::size_t b = bestByEv(opts, p);
        if (b != prev) {
            if (prev != opts.size()) {
                std::cout << "  p from " << from << " to " << (k - 1) / 1000.0 << ": "
                          << opts[prev].name << '\n';
            }
            prev = b;
            from = p0;
        }
    }
    std::cout << "  p from " << from << " to 1.000: " << opts[prev].name << '\n';
    return 0;
}
