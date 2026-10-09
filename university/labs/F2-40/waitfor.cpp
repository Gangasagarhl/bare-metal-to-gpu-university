// Worked-example check (F2-40): build the wait-for graph from "holds / waits for" lines
// (as read from a thread dump) and look for a cycle.
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

struct ThreadState
{
    std::string thread;
    std::string holds;
    std::string waitsFor;
};

int main()
{
    const std::vector<ThreadState> dump = {
        {"T1", "A", "B"},
        {"T2", "B", "C"},
        {"T3", "C", "A"},
        {"T4", "D", "A"},
    };
    std::map<std::string, std::string> owner;  // lock -> thread holding it
    for (const auto& s : dump) {
        owner[s.holds] = s.thread;
    }
    std::map<std::string, std::string> waitsOn;  // thread -> thread it waits for
    for (const auto& s : dump) {
        waitsOn[s.thread] = owner[s.waitsFor];
        std::cout << s.thread << " waits for lock " << s.waitsFor << ", held by " << waitsOn[s.thread] << '\n';
    }
    for (const auto& s : dump) {
        std::set<std::string> seen;
        std::string t = s.thread;
        std::string path = t;
        while (waitsOn.count(t) != 0 && seen.insert(t).second) {
            t = waitsOn[t];
            path += " -> " + t;
        }
        const bool stuck = waitsOn.count(t) != 0;  // the walk stopped on a thread seen before
        std::cout << "from " << s.thread << ": " << path
                  << (t == s.thread ? "  (back to the start: CYCLE)"
                                    : (stuck ? "  (not in the cycle, but waits on it)" : "  (ends: no cycle)"))
                  << '\n';
    }
    return 0;
}
