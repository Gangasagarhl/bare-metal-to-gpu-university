// stack.cpp - F9-65: check a robot software stack description before building it.
// Input (stdin), one item per line:
//   component NAME LAYER CPU PERIOD_US BUDGET_US CRITICALITY      (CRITICALITY: high | low)
//   chain NAME COMPONENT COMPONENT ...                             (a sensor-to-actuator path)
// Output: the stack by layer; per-CPU utilisation U = sum(budget / period) with the
// rate-monotonic utilisation bound n(2^(1/n) - 1) (a sufficient test, Liu and Layland);
// exact response-time analysis per component under rate-monotonic priorities (shorter period =
// higher priority): R = C + sum over higher-priority j of ceil(R / T_j) * C_j, iterated;
// a warning where high- and low-criticality work share a CPU; and, for each chain, a simple
// worst-case data-age estimate: the sum over the chain of (period + response time).
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

struct Component {
    std::string name, layer;
    int cpu = 0;
    long period_us = 0, budget_us = 0;
    bool high = false;
    long response_us = -1;   // -1: misses its deadline (no fixed point within the period)
};

int main()
{
    const std::vector<std::string> layer_order = {"hardware_io", "drivers", "middleware", "estimation",
                                                  "control", "perception", "planning", "operations"};
    std::vector<Component> comps;
    std::map<std::string, size_t> by_name;
    std::vector<std::vector<std::string>> chains;
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream in(line);
        std::string kind;
        in >> kind;
        if (kind == "component") {
            Component c;
            std::string crit;
            in >> c.name >> c.layer >> c.cpu >> c.period_us >> c.budget_us >> crit;
            c.high = crit == "high";
            by_name[c.name] = comps.size();
            comps.push_back(c);
        } else if (kind == "chain") {
            std::vector<std::string> ch;
            std::string w;
            while (in >> w) {
                ch.push_back(w);
            }
            chains.push_back(ch);
        }
    }

    std::cout << "== the stack, bottom layer first\n";
    for (const auto& layer : layer_order) {
        std::cout << std::left << std::setw(13) << layer << ':';
        for (const auto& c : comps) {
            if (c.layer == layer) {
                std::cout << ' ' << c.name << "(cpu" << c.cpu << (c.high ? ",high" : "") << ')';
            }
        }
        std::cout << '\n';
    }

    std::cout << "== CPU budgets\n";
    int problems = 0;
    std::map<int, std::vector<const Component*>> per_cpu;
    for (const auto& c : comps) {
        per_cpu[c.cpu].push_back(&c);
    }
    for (const auto& [cpu, list] : per_cpu) {
        double u = 0;
        bool any_high = false, any_low = false;
        for (const Component* c : list) {
            u += static_cast<double>(c->budget_us) / static_cast<double>(c->period_us);
            any_high = any_high || c->high;
            any_low = any_low || !c->high;
        }
        double n = static_cast<double>(list.size());
        double bound = n * (std::pow(2.0, 1.0 / n) - 1.0);
        std::cout << "cpu" << cpu << ": " << list.size() << " components, U = " << std::fixed
                  << std::setprecision(3) << u << ", RM bound " << bound << " -> ";
        if (u > 1.0) {
            std::cout << "OVERLOADED: no scheduler can meet every deadline\n";
            ++problems;
        } else if (u > bound) {
            std::cout << "above the bound: needs exact response-time analysis or measurement\n";
        } else {
            std::cout << "schedulable by rate-monotonic priorities (sufficient test passed)\n";
        }
        if (any_high && any_low) {
            std::cout << "  warning: high- and low-criticality components share cpu" << cpu << '\n';
        }
        // response-time analysis, highest priority (shortest period) first
        std::vector<const Component*> prio = list;
        std::stable_sort(prio.begin(), prio.end(),
                         [](const Component* a, const Component* b) { return a->period_us < b->period_us; });
        for (size_t i = 0; i < prio.size(); ++i) {
            long r = prio[i]->budget_us;
            while (true) {
                long next = prio[i]->budget_us;
                for (size_t j = 0; j < i; ++j) {
                    next += (r + prio[j]->period_us - 1) / prio[j]->period_us * prio[j]->budget_us;
                }
                if (next == r || next > prio[i]->period_us) {
                    r = next;
                    break;
                }
                r = next;
            }
            Component& c = comps[by_name[prio[i]->name]];
            c.response_us = r <= c.period_us ? r : -1;
            std::cout << "  " << std::left << std::setw(16) << c.name << std::right << " T " << std::setw(6)
                      << c.period_us << "  C " << std::setw(5) << c.budget_us << "  R ";
            if (c.response_us < 0) {
                std::cout << "> T: MISSES ITS DEADLINE\n";
                ++problems;
            } else {
                std::cout << std::setw(6) << c.response_us << '\n';
            }
        }
    }

    std::cout << "== sensor-to-actuator chains (estimate: sum of period + response time)\n";
    for (const auto& ch : chains) {
        long age = 0;
        bool bad = false;
        std::cout << ch[0] << ':';
        for (size_t i = 1; i < ch.size(); ++i) {
            auto it = by_name.find(ch[i]);
            if (it == by_name.end()) {
                std::cout << " [unknown component " << ch[i] << "]";
                ++problems;
                continue;
            }
            const Component& c = comps[it->second];
            if (c.response_us < 0) {
                bad = true;
            }
            age += c.period_us + c.response_us;
            std::cout << ' ' << c.name;
        }
        if (bad) {
            std::cout << " -> no bound: a component on the chain misses its deadline\n";
        } else {
            std::cout << " -> worst-case data age about " << age << " us\n";
        }
    }
    std::cout << "problems: " << problems << '\n';
    return problems == 0 ? 0 : 1;
}
