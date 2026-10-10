// mcdc.cpp - F11-22: how much does a test suite exercise one decision?
// Decision (the drive-enable rule of the course robot's arm):
//   enable = !estop && wd_ok && (auto_mode || deadman)
// For a test suite (a list of input vectors) the program reports:
//   decision coverage  : the decision was seen both true and false
//   condition coverage : every condition was seen both true and false
//   MC/DC (unique cause): for every condition there are two tests that differ ONLY in that
//                        condition and give different decisions (it independently affects it)
// Then it searches all subsets of the 16 possible vectors for the smallest MC/DC suite.
#include <bit>
#include <cstdio>
#include <string>
#include <vector>

constexpr int kConditions = 4;
const char* const kNames[kConditions] = {"estop", "wd_ok", "auto_mode", "deadman"};

bool decision(unsigned v)   // bit 0 estop, bit 1 wd_ok, bit 2 auto_mode, bit 3 deadman
{
    const bool estop = v & 1u, wd_ok = v & 2u, auto_mode = v & 4u, deadman = v & 8u;
    return !estop && wd_ok && (auto_mode || deadman);
}

struct Coverage {
    bool decision = false, condition = false, mcdc = false;
    std::string missing;
};

Coverage measure(const std::vector<unsigned>& suite)
{
    Coverage c;
    bool seen_t = false, seen_f = false;
    unsigned ones = 0, zeros = 0;
    for (unsigned v : suite) {
        (decision(v) ? seen_t : seen_f) = true;
        ones |= v;
        zeros |= ~v & 0xFu;
    }
    c.decision = seen_t && seen_f;
    c.condition = ones == 0xFu && zeros == 0xFu;
    c.mcdc = true;
    for (int k = 0; k < kConditions; ++k) {
        bool shown = false;
        for (unsigned a : suite) {
            for (unsigned b : suite) {
                shown = shown || ((a ^ b) == (1u << k) && decision(a) != decision(b));
            }
        }
        if (!shown) {
            c.mcdc = false;
            c.missing += std::string(c.missing.empty() ? "" : ", ") + kNames[k];
        }
    }
    return c;
}

void report(const char* name, const std::vector<unsigned>& suite)
{
    const Coverage c = measure(suite);
    std::printf("%-30s %2zu tests | decision %-3s | condition %-3s | MC/DC %-3s", name,
                suite.size(), c.decision ? "yes" : "no", c.condition ? "yes" : "no",
                c.mcdc ? "yes" : "no");
    if (!c.mcdc) {
        std::printf(" (not shown: %s)", c.missing.c_str());
    }
    std::puts("");
}

int main()
{
    std::puts("vectors: estop wd_ok auto_mode deadman -> enable");
    for (unsigned v = 0; v < 16; ++v) {
        std::printf("  v%-2u    %d     %d       %d        %d    ->   %d\n", v, v & 1u,
                    (v >> 1) & 1u, (v >> 2) & 1u, (v >> 3) & 1u, decision(v) ? 1 : 0);
    }
    report("A: one pass, one e-stop", {6u, 7u});
    report("B: all false, all true", {0u, 15u});
    report("C: B plus 'auto, no deadman'", {0u, 15u, 6u});
    report("D: the team's 6 tests", {6u, 7u, 4u, 10u, 0u, 15u});
    unsigned best = 0;
    for (unsigned mask = 1; mask < (1u << 16); ++mask) {
        if (best != 0 && std::popcount(mask) >= std::popcount(best)) {
            continue;
        }
        std::vector<unsigned> suite;
        for (unsigned v = 0; v < 16; ++v) {
            if (mask & (1u << v)) {
                suite.push_back(v);
            }
        }
        if (measure(suite).mcdc) {
            best = mask;
        }
    }
    std::vector<unsigned> minimal;
    std::string list;
    for (unsigned v = 0; v < 16; ++v) {
        if (best & (1u << v)) {
            minimal.push_back(v);
            list += " v" + std::to_string(v);
        }
    }
    std::printf("smallest MC/DC suite found by exhaustive search:%s\n", list.c_str());
    report("E: smallest MC/DC suite", minimal);
    return 0;
}
