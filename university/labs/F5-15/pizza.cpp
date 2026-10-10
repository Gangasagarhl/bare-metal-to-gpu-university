// Three friends must agree on one pizza. Three simple protocols, each run
// 1000 times on the simulated network with lost and late messages and an
// occasional crash; the checker tests agreement, validity and termination.
#include <cstdio>
#include <map>
#include <string>
#include <vector>

#include "sim.h"

const std::vector<std::string> kPizza = {"tomato", "mushroom", "spinach"};
constexpr int kFriends = 3;
constexpr sim::Time kEnd = 1000;  // ms of simulated time per run

struct Msg
{
    int from = 0;
    int to = 0;
    int pizza = 0;
};

enum class Protocol { WaitAWhile, WaitForAll, AskOrganiser };

struct Friend
{
    int proposal = 0;
    std::map<int, int> heard;  // friend -> their proposal (including myself)
    int decision = -1;         // -1: not decided yet
    bool crashed = false;
};

// Majority of what I heard; a tie goes to the lowest pizza number.
static int majority(const std::map<int, int>& heard)
{
    std::vector<int> votes(kPizza.size(), 0);
    for (const auto& [who, p] : heard) {
        ++votes[p];
    }
    int best = 0;
    for (int p = 1; p < static_cast<int>(votes.size()); ++p) {
        if (votes[p] > votes[best]) {
            best = p;
        }
    }
    return best;
}

struct Outcome
{
    bool agreement = true;
    bool validity = true;
    bool termination = true;
    std::vector<Friend> friends;
};

static Outcome runOnce(Protocol proto, std::uint64_t seed)
{
    sim::Rng rng(seed);
    sim::NetOptions net;
    net.minDelay = 1;
    net.maxDelay = 80;
    net.dropPercent = 10;
    sim::Network<Msg> network(net, rng);

    std::vector<Friend> f(kFriends + 1);  // f[1..3]
    for (int i = 1; i <= kFriends; ++i) {
        f[i].proposal = static_cast<int>(rng.range(0, 2));
        f[i].heard[i] = f[i].proposal;
    }
    // In one run out of three, one friend's phone dies at a random moment.
    const int victim = rng.percent(33) ? static_cast<int>(rng.range(1, kFriends)) : 0;
    const sim::Time crashAt = rng.range(0, 60);

    auto tell = [&](int from, int to, int pizza, sim::Time now) {
        if (from != to) {
            network.send(now, Msg{from, to, pizza});
        }
    };
    for (sim::Time now = 0; now <= kEnd; ++now) {
        if (victim != 0 && now == crashAt) {
            f[victim].crashed = true;
        }
        if (now == 0) {  // everyone (or only the organiser) speaks first
            for (int i = 1; i <= kFriends; ++i) {
                for (int j = 1; j <= kFriends; ++j) {
                    if (proto != Protocol::AskOrganiser && !f[i].crashed) {
                        tell(i, j, f[i].proposal, now);
                    }
                }
            }
            if (proto == Protocol::AskOrganiser && !f[1].crashed) {
                f[1].decision = f[1].proposal;  // the organiser simply decides
                for (int j = 2; j <= kFriends; ++j) {
                    tell(1, j, f[1].decision, now);
                }
            }
        }
        for (const Msg& m : network.due(now)) {
            Friend& me = f[m.to];
            if (me.crashed || me.decision >= 0) {
                continue;
            }
            me.heard[m.from] = m.pizza;
            if (proto == Protocol::WaitForAll && static_cast<int>(me.heard.size()) == kFriends) {
                me.decision = majority(me.heard);
            }
            if (proto == Protocol::AskOrganiser) {
                me.decision = m.pizza;
            }
        }
        if (proto == Protocol::WaitAWhile && now == 50) {  // "if you have not heard by then..."
            for (int i = 1; i <= kFriends; ++i) {
                if (!f[i].crashed) {
                    f[i].decision = majority(f[i].heard);
                }
            }
        }
    }

    Outcome o;
    int agreed = -1;
    for (int i = 1; i <= kFriends; ++i) {
        const Friend& x = f[i];
        if (x.decision >= 0) {
            if (agreed >= 0 && x.decision != agreed) {
                o.agreement = false;
            }
            agreed = x.decision;
            bool proposed = false;
            for (int j = 1; j <= kFriends; ++j) {
                proposed = proposed || f[j].proposal == x.decision;
            }
            o.validity = o.validity && proposed;
        } else if (!x.crashed) {
            o.termination = false;  // a working friend never decided
        }
    }
    o.friends = f;
    return o;
}

static void printRun(const Outcome& o)
{
    for (int i = 1; i <= kFriends; ++i) {
        const Friend& x = o.friends[i];
        std::printf("    friend %d proposed %-8s heard from:", i, kPizza[x.proposal].c_str());
        for (const auto& [who, p] : x.heard) {
            std::printf(" %d(%s)", who, kPizza[p].c_str());
        }
        std::printf("  -> %s%s\n", x.decision >= 0 ? kPizza[x.decision].c_str() : "no decision",
                    x.crashed ? " [crashed]" : "");
    }
}

int main()
{
    const std::vector<std::pair<Protocol, const char*>> protocols = {
        {Protocol::WaitAWhile, "A: vote, decide after 50 ms"},
        {Protocol::WaitForAll, "B: wait for all three votes"},
        {Protocol::AskOrganiser, "C: friend 1 decides alone"},
    };
    std::printf("%-30s %10s %10s %12s   (runs out of 1000 that broke it)\n", "protocol",
                "agreement", "validity", "termination");
    for (const auto& [proto, name] : protocols) {
        int a = 0;
        int v = 0;
        int t = 0;
        std::uint64_t example = 0;
        for (std::uint64_t seed = 1; seed <= 1000; ++seed) {
            const Outcome o = runOnce(proto, seed);
            a += !o.agreement;
            v += !o.validity;
            t += !o.termination;
            if (example == 0 && (!o.agreement || !o.termination)) {
                example = seed;
            }
        }
        std::printf("%-30s %10d %10d %12d\n", name, a, v, t);
        if (example != 0) {
            std::printf("  first broken run, seed %llu:\n", static_cast<unsigned long long>(example));
            printRun(runOnce(proto, example));
        }
    }
    return 0;
}
