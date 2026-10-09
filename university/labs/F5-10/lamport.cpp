// lamport.cpp - Lamport clocks in a three-node simulation (DS301 course lab, chapter F5-10).
// Three nodes A, B, C exchange messages. The simulator knows the true time of every event;
// each node only has its own physical clock, which is off by a fixed offset. We compare
// three ways of ordering the events: true time, the nodes' physical clocks, Lamport clocks.
#include <algorithm>
#include <cstdio>
#include <map>
#include <string>
#include <vector>

enum class Kind { local, send, receive };

struct Event
{
    int trueMs;        // known only to the simulator
    char node;         // 'A', 'B' or 'C'
    Kind kind;
    std::string name;
    int msg = 0;       // message number for send / receive
    int clockMs = 0;   // the node's physical clock reading when the event happened
    int lamport = 0;   // the node's Lamport clock after the event
};

int offsetMs(char node)   // physical clock error of each node
{
    switch (node) {
    case 'A': return 0;
    case 'B': return -40;   // B's clock is 40 ms behind
    default:  return 25;    // C's clock is 25 ms ahead
    }
}

int main()
{
    // The scenario, in true-time order (a receive always comes after its send).
    std::vector<Event> ev = {
        {1000, 'A', Kind::local,   "a1 write x=1"},
        {1005, 'A', Kind::send,    "a2 send m1 to B", 1},
        {1010, 'C', Kind::local,   "c1 write y=7"},
        {1017, 'B', Kind::receive, "b1 recv m1", 1},
        {1020, 'B', Kind::local,   "b2 write x=2"},
        {1022, 'B', Kind::send,    "b3 send m2 to C", 2},
        {1030, 'C', Kind::receive, "c2 recv m2", 2},
        {1031, 'C', Kind::send,    "c3 send m3 to A", 3},
        {1040, 'A', Kind::local,   "a3 read x"},
        {1045, 'A', Kind::receive, "a4 recv m3", 3},
        {1050, 'B', Kind::send,    "b4 send m4 to A", 4},
        {1063, 'A', Kind::receive, "a5 recv m4", 4},
    };

    // Lamport's rules: tick before every event; a send carries the clock; a receive
    // sets the clock to max(own, carried) and then ticks.
    std::map<char, int> clock;          // Lamport clock of each node, starting at 0
    std::map<int, int> carried;         // message -> Lamport time it carries
    for (Event& e : ev) {
        e.clockMs = e.trueMs + offsetMs(e.node);
        int& c = clock[e.node];
        if (e.kind == Kind::receive) {
            c = std::max(c, carried.at(e.msg));
        }
        c += 1;
        e.lamport = c;
        if (e.kind == Kind::send) {
            carried[e.msg] = c;
        }
    }

    std::printf("%-17s %4s %8s %10s %8s\n", "event", "node", "true ms", "clock ms", "Lamport");
    for (const Event& e : ev) {
        std::printf("%-17s %4c %8d %10d %8d\n", e.name.c_str(), e.node, e.trueMs, e.clockMs,
                    e.lamport);
    }

    // List the messages whose receive an ordering places before the matching send.
    auto violations = [&](const std::vector<Event>& order) {
        std::map<int, std::size_t> sendPos;
        std::map<int, std::size_t> recvPos;
        for (std::size_t i = 0; i < order.size(); ++i) {
            if (order[i].kind == Kind::send) {
                sendPos[order[i].msg] = i;
            }
            if (order[i].kind == Kind::receive) {
                recvPos[order[i].msg] = i;
            }
        }
        std::string bad;
        for (const auto& [m, r] : recvPos) {
            if (r < sendPos.at(m)) {
                bad += " m" + std::to_string(m);
            }
        }
        return bad.empty() ? std::string(" none") : bad;
    };
    auto show = [&](const char* title, std::vector<Event> order, auto key) {
        std::stable_sort(order.begin(), order.end(),
                         [&](const Event& x, const Event& y) { return key(x) < key(y); });
        std::printf("\n%s:\n ", title);
        for (const Event& e : order) {
            std::printf(" %s", e.name.substr(0, 2).c_str());
        }
        std::printf("\n  receives placed before their send:%s\n", violations(order).c_str());
    };
    show("order by true time (only the simulator knows it)", ev,
         [](const Event& e) { return e.trueMs; });
    show("order by the nodes' physical clocks", ev,
         [](const Event& e) { return e.clockMs; });
    show("order by (Lamport time, node name)", ev,
         [](const Event& e) { return e.lamport * 256 + e.node; });
    return 0;
}
