// fault_tree.cpp - F11-19: a small fault tree, evaluated three ways.
// Input (stdin):
//   EVENT <name> <probability per demand>        a basic event (assumed independent of the others)
//   GATE  <name> AND|OR <input> <input> ...      a gate over events or other gates
//   TOP   <name>                                 the top event
// Output: (1) the minimal cut sets, (2) the "gate arithmetic" probability that multiplies and
// combines the inputs of each gate as if they were independent, (3) the exact probability,
// computed by enumerating every combination of basic events (fine for up to 20 events).
// When a basic event appears under two branches, (2) is wrong and (3) shows by how much.
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

struct Gate {
    bool is_and = false;
    std::vector<std::string> inputs;
};

std::map<std::string, double> events;
std::map<std::string, Gate> gates;

using CutSet = std::set<std::string>;

std::vector<CutSet> cut_sets(const std::string& node)
{
    if (events.count(node) != 0) {
        return {CutSet{node}};
    }
    const Gate& g = gates.at(node);
    std::vector<CutSet> result;
    if (!g.is_and) {                           // OR: any input's cut set is a cut set
        for (const auto& in : g.inputs) {
            for (auto& c : cut_sets(in)) {
                result.push_back(c);
            }
        }
    } else {                                   // AND: one cut set from every input, merged
        result = {CutSet{}};
        for (const auto& in : g.inputs) {
            std::vector<CutSet> next;
            for (const auto& a : result) {
                for (const auto& b : cut_sets(in)) {
                    CutSet m = a;
                    m.insert(b.begin(), b.end());
                    next.push_back(m);
                }
            }
            result = next;
        }
    }
    std::vector<CutSet> minimal;               // drop duplicates and supersets
    for (const auto& c : result) {
        bool redundant = false;
        for (const auto& d : result) {
            const bool subset = std::includes(c.begin(), c.end(), d.begin(), d.end());
            if (subset && (d.size() < c.size() || (d == c && &d < &c))) {
                redundant = true;
            }
        }
        if (!redundant) {
            minimal.push_back(c);
        }
    }
    return minimal;
}

double gate_arithmetic(const std::string& node)
{
    if (events.count(node) != 0) {
        return events.at(node);
    }
    const Gate& g = gates.at(node);
    double p = g.is_and ? 1.0 : 0.0;
    for (const auto& in : g.inputs) {
        const double q = gate_arithmetic(in);
        p = g.is_and ? p * q : 1.0 - (1.0 - p) * (1.0 - q);
    }
    return p;
}

bool occurs(const std::string& node, const std::map<std::string, bool>& state)
{
    if (events.count(node) != 0) {
        return state.at(node);
    }
    const Gate& g = gates.at(node);
    for (const auto& in : g.inputs) {
        const bool v = occurs(in, state);
        if (g.is_and && !v) {
            return false;
        }
        if (!g.is_and && v) {
            return true;
        }
    }
    return g.is_and;
}

int main()
{
    std::string line, top;
    while (std::getline(std::cin, line)) {
        std::istringstream in(line);
        std::string kind, name;
        if (!(in >> kind) || kind[0] == '#') {
            continue;
        }
        in >> name;
        if (kind == "EVENT") {
            in >> events[name];
        } else if (kind == "GATE") {
            std::string type, input;
            in >> type;
            Gate g;
            g.is_and = (type == "AND");
            while (in >> input) {
                g.inputs.push_back(input);
            }
            gates[name] = g;
        } else if (kind == "TOP") {
            top = name;
        }
    }
    std::printf("Top event: %s\n\nMinimal cut sets (each is enough on its own):\n", top.c_str());
    double rare = 0.0;
    for (const auto& c : cut_sets(top)) {
        double p = 1.0;
        std::string names;
        for (const auto& e : c) {
            p *= events.at(e);
            names += (names.empty() ? "" : " AND ") + e;
        }
        rare += p;
        std::printf("  {%s}  p = %.3g\n", names.c_str(), p);
    }
    std::vector<std::string> names;
    for (const auto& [n, p] : events) {
        names.push_back(n);
    }
    double exact = 0.0;
    for (unsigned long mask = 0; mask < (1UL << names.size()); ++mask) {
        std::map<std::string, bool> state;
        double p = 1.0;
        for (std::size_t i = 0; i < names.size(); ++i) {
            const bool on = ((mask >> i) & 1UL) != 0;
            state[names[i]] = on;
            p *= on ? events.at(names[i]) : 1.0 - events.at(names[i]);
        }
        if (occurs(top, state)) {
            exact += p;
        }
    }
    std::printf("\nSum over minimal cut sets (rare-event approximation): %.4g\n", rare);
    std::printf("Gate arithmetic (assumes every gate's inputs independent): %.4g\n",
                gate_arithmetic(top));
    std::printf("Exact, by enumerating all %zu basic events:               %.4g\n",
                names.size(), exact);
    return 0;
}
