// vclock.cpp - vector clocks on the same three-node scenario as lamport.cpp.
// A Lamport clock gives an order that respects causality, but it cannot tell you whether
// two events were causally related. A vector clock can: e -> f exactly when V(e) < V(f).
#include <algorithm>
#include <array>
#include <cstdio>
#include <map>
#include <string>
#include <vector>

enum class Kind { local, send, receive };
using VC = std::array<int, 3>;   // one counter per node: A, B, C

struct Event
{
    char node;
    Kind kind;
    std::string name;
    int msg = 0;
    VC v{};
};

bool leq(const VC& x, const VC& y)
{
    for (int i = 0; i < 3; ++i) {
        if (x[i] > y[i]) {
            return false;
        }
    }
    return true;
}

std::string relation(const Event& e, const Event& f)
{
    if (e.v == f.v) {
        return "same event";
    }
    if (leq(e.v, f.v)) {
        return e.name.substr(0, 2) + " happened before " + f.name.substr(0, 2);
    }
    if (leq(f.v, e.v)) {
        return f.name.substr(0, 2) + " happened before " + e.name.substr(0, 2);
    }
    return "concurrent";
}

int main()
{
    std::vector<Event> ev = {   // same scenario and order as lamport.cpp
        {'A', Kind::local, "a1 write x=1"},   {'A', Kind::send, "a2 send m1 to B", 1},
        {'C', Kind::local, "c1 write y=7"},   {'B', Kind::receive, "b1 recv m1", 1},
        {'B', Kind::local, "b2 write x=2"},   {'B', Kind::send, "b3 send m2 to C", 2},
        {'C', Kind::receive, "c2 recv m2", 2}, {'C', Kind::send, "c3 send m3 to A", 3},
        {'A', Kind::local, "a3 read x"},      {'A', Kind::receive, "a4 recv m3", 3},
        {'B', Kind::send, "b4 send m4 to A", 4}, {'A', Kind::receive, "a5 recv m4", 4},
    };
    std::map<char, VC> clock;
    std::map<int, VC> carried;
    for (Event& e : ev) {
        const int me = e.node - 'A';
        VC& c = clock[e.node];
        if (e.kind == Kind::receive) {
            const VC& m = carried.at(e.msg);
            for (int i = 0; i < 3; ++i) {
                c[i] = std::max(c[i], m[i]);
            }
        }
        c[me] += 1;
        e.v = c;
        if (e.kind == Kind::send) {
            carried[e.msg] = c;
        }
        std::printf("%-17s [A=%d B=%d C=%d]\n", e.name.c_str(), c[0], c[1], c[2]);
    }
    auto find = [&](const char* id) -> const Event& {
        for (const Event& e : ev) {
            if (e.name.rfind(id, 0) == 0) {
                return e;
            }
        }
        return ev.front();
    };
    std::printf("\n");
    const char* pairs[][2] = {{"a1", "b2"}, {"a1", "c1"}, {"c1", "b2"}, {"a3", "b2"},
                              {"a3", "a4"}, {"b4", "c3"}, {"c1", "a4"}};
    for (const auto& p : pairs) {
        std::printf("%s vs %s: %s\n", p[0], p[1], relation(find(p[0]), find(p[1])).c_str());
    }
    return 0;
}
