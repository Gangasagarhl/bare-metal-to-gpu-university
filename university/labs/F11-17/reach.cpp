// F11-17 Listing 2: who can reach what in a cluster's networks. Reads the allowed
// connections ("allow A B service": A may open a connection to B), finds the shortest
// chain of hops from the start node to every target, then applies the "fix" lines
// (remove one allowed connection each) one by one, searching again after each.
// A chain of hops is not an attack by itself: each hop also needs a weakness on the
// node it reaches (the STRIDE threats of that node).
#include <cstdio>
#include <iostream>
#include <map>
#include <queue>
#include <sstream>
#include <string>
#include <vector>

struct Edge
{
    std::string to, service;
    bool removed = false;
};

using Graph = std::map<std::string, std::vector<Edge>>;

void search(const Graph& g, const std::string& start, const std::vector<std::string>& targets)
{
    std::map<std::string, std::string> prev;     // node -> "previous node|service"
    std::queue<std::string> q;
    q.push(start);
    prev[start] = "";
    while (!q.empty()) {
        const std::string n = q.front();
        q.pop();
        const auto it = g.find(n);
        if (it == g.end()) {
            continue;
        }
        for (const Edge& e : it->second) {
            if (!e.removed && prev.count(e.to) == 0) {
                prev[e.to] = n + "|" + e.service;
                q.push(e.to);
            }
        }
    }
    std::printf("  reachable from %s: %zu nodes\n", start.c_str(), prev.size() - 1);
    for (const std::string& t : targets) {
        if (prev.count(t) == 0) {
            std::printf("  %-12s not reachable\n", t.c_str());
            continue;
        }
        std::vector<std::string> hops;
        for (std::string n = t; n != start;) {
            const std::string& p = prev[n];
            const std::size_t bar = p.find('|');
            hops.push_back(p.substr(0, bar) + " =" + p.substr(bar + 1) + "=> " + n);
            n = p.substr(0, bar);
        }
        std::printf("  %-12s %zu hops:", t.c_str(), hops.size());
        for (auto h = hops.rbegin(); h != hops.rend(); ++h) {
            std::printf("  %s", h->c_str());
        }
        std::printf("\n");
    }
}

int main()
{
    Graph g;
    std::string start;
    std::vector<std::string> targets;
    std::vector<std::pair<std::string, std::string>> fixes;
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream in(line);
        std::string word, a, b, svc;
        in >> word;
        if (word == "allow") {
            in >> a >> b >> svc;
            g[a].push_back({b, svc});
        } else if (word == "start") {
            in >> start;
        } else if (word == "target") {
            in >> a;
            targets.push_back(a);
        } else if (word == "fix") {
            in >> a >> b;
            fixes.push_back({a, b});
        }
    }
    std::printf("before the fixes\n");
    search(g, start, targets);
    for (const auto& [a, b] : fixes) {
        bool found = false;
        for (Edge& e : g[a]) {
            if (e.to == b) {
                e.removed = true;
                found = true;
            }
        }
        std::printf("after fix: remove %s -> %s%s\n", a.c_str(), b.c_str(),
                    found ? "" : " (NO SUCH RULE)");
        if (!found) {
            return 1;
        }
        search(g, start, targets);
    }
    return 0;
}
