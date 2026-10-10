// F6-12 Listing 1: exhaustive interleaving explorer for a 4-thread "block".
// Each thread runs a tiny program of shared-memory writes (W), reads (R) and barriers (B).
// The explorer tries EVERY order in which the threads' steps can interleave (a barrier lets nobody
// past until all four threads have reached it) and counts the orders that give a wrong answer.
// Thread t writes s[t] = 10*k + t in round k and reads its neighbour s[(t+1)%4], expecting
// 10*k + (t+1)%4. This is a model of the rules, not of any GPU's timing.
#include <array>
#include <cstdio>
#include <string>
#include <vector>

constexpr int T = 4;

struct Counter
{
    long schedules = 0;
    long wrong = 0;
};

struct State
{
    std::array<int, T> pc{};
    std::array<int, T> mem{};
    bool ok = true;
};

void explore(const std::string& prog, State st, Counter& c)
{
    const int len = static_cast<int>(prog.size());
    bool allDone = true;
    for (int t = 0; t < T; ++t) { allDone = allDone && st.pc[t] == len; }
    if (allDone) {
        ++c.schedules;
        if (!st.ok) { ++c.wrong; }
        return;
    }
    // a barrier is passed by everybody at once, when everybody is waiting at it
    bool allAtBarrier = true;
    for (int t = 0; t < T; ++t) {
        allAtBarrier = allAtBarrier && st.pc[t] < len && prog[st.pc[t]] == 'B' && st.pc[t] == st.pc[0];
    }
    if (allAtBarrier) {
        for (int t = 0; t < T; ++t) { ++st.pc[t]; }
        explore(prog, st, c);
        return;
    }
    for (int t = 0; t < T; ++t) {
        if (st.pc[t] == len || prog[st.pc[t]] == 'B') { continue; }   // finished, or waiting
        State next = st;
        int round = 0;                                             // which W/R pair this is
        for (int i = 0; i < st.pc[t]; ++i) { if (prog[i] == 'R') { ++round; } }
        if (prog[st.pc[t]] == 'W') {
            next.mem[t] = 10 * (round + 1) + t;
        } else {                                                   // 'R'
            int nb = (t + 1) % T;
            if (next.mem[nb] != 10 * (round + 1) + nb) { next.ok = false; }
        }
        ++next.pc[t];
        explore(prog, next, c);
    }
}

int main()
{
    const std::vector<std::pair<std::string, std::string>> programs = {
        {"WR", "one round, no barrier"},
        {"WBR", "one round, barrier between write and read"},
        {"WBRWBR", "two rounds, barrier only after each write"},
        {"WBRBWBR", "two rounds, barriers after the write AND after the read"},
    };
    std::printf("%-9s %-52s %-10s %s\n", "program", "meaning", "orders", "wrong orders");
    for (const auto& p : programs) {
        Counter c;
        explore(p.first, State{}, c);
        std::printf("%-9s %-52s %-10ld %ld\n", p.first.c_str(), p.second.c_str(), c.schedules, c.wrong);
    }
    return 0;
}
