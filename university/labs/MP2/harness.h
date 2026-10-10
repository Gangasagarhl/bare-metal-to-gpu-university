// harness.h - MP2 milestone 1 starter: a deterministic fault-injection harness.
// Scenarios are data (read from a text file such as suite.in); each one runs on
// the DS302 Raft library (raft.h, cluster.h, copied unchanged from labs/F5-21)
// and is judged by four checks: Raft safety, linearizability of the clients'
// history, liveness after the faults stop, and determinism (same seed, same run).
#pragma once
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <istream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "cluster.h"
#include "linearizability.h"

namespace mp2 {

using raft::Time;

// ---- scenarios: the fault schedule is data, not code ----------------------------

struct Step
{
    Time at = 0;
    std::string verb;               // crash restart isolate reconnect partition heal
    std::vector<std::string> args;  // targets: an id, leader, follower, down, all
};

struct Scenario
{
    std::string name;
    int servers = 5;
    std::uint64_t firstSeed = 1;
    std::uint64_t lastSeed = 1;
    sim::NetOptions net;
    int clients = 2;
    int maxOps = 50;            // the checker handles at most 63 operations
    std::vector<Step> steps;
    std::string nemesis;        // "", "random" or "hunter"
    Time nemFrom = 0;
    Time nemTo = 0;
    Time nemEvery = 0;
    Time settle = 3000;         // heal the network and restart every crashed server
    Time end = 4000;            // stop; liveness probes must be answered by now
};

// What one run changes in the library: a planted bug, or an option such as the no-op rule.
struct Variant
{
    std::string name = "correct";
    raft::Faults faults;
    raft::Options options;  // the library's defaults unless a variant changes them
};

inline std::vector<Scenario> parseScenarios(std::istream& in)
{
    std::vector<Scenario> out;
    std::string line;
    int lineNo = 0;
    while (std::getline(in, line)) {
        ++lineNo;
        line = line.substr(0, line.find('#'));
        std::istringstream words(line);
        std::string key;
        if (!(words >> key)) {
            continue;  // empty line or comment
        }
        auto fail = [&](const std::string& why) {
            throw std::runtime_error("suite line " + std::to_string(lineNo) + ": " + why);
        };
        if (key == "scenario") {
            out.emplace_back();
            words >> out.back().name;
            continue;
        }
        if (out.empty()) {
            fail("'" + key + "' before the first 'scenario'");
        }
        Scenario& s = out.back();
        if (key == "servers") {
            words >> s.servers;
        } else if (key == "seed") {
            words >> s.firstSeed;
            s.lastSeed = s.firstSeed;
        } else if (key == "seeds") {
            words >> s.firstSeed >> s.lastSeed;
        } else if (key == "net") {
            words >> s.net.minDelay >> s.net.maxDelay >> s.net.dropPercent >>
                s.net.duplicatePercent;
        } else if (key == "clients") {
            words >> s.clients >> s.maxOps;
        } else if (key == "nemesis") {
            words >> s.nemesis >> s.nemFrom >> s.nemTo >> s.nemEvery;
        } else if (key == "settle") {
            words >> s.settle;
        } else if (key == "end") {
            words >> s.end;
        } else if (key == "at") {
            Step st;
            words >> st.at >> st.verb;
            for (std::string a; words >> a;) {
                st.args.push_back(a);
            }
            s.steps.push_back(st);
        } else {
            fail("unknown keyword '" + key + "'");
        }
        if (words.fail() && !words.eof()) {
            fail("bad number after '" + key + "'");
        }
        if (s.maxOps > 50) {
            fail("at most 50 client operations per run (checker limit)");
        }
    }
    return out;
}

// ---- a cluster that also records what the observability milestone will need ----

class Observed : public raft::Cluster
{
public:
    using raft::Cluster::Cluster;

    void trace(int node, int term, raft::Role role, const std::string& text) override
    {
        if (text.rfind("election timeout", 0) == 0) {
            electionTimes.push_back(now());
        }
        if (now() >= echoFrom && now() <= echoTo) {
            std::printf("%5lld ms  n%d  term %-2d %-9s  %s\n", static_cast<long long>(now()),
                        node, term, raft::roleName(role), text.c_str());
        }
    }

    void becameLeader(int node, int term) override
    {
        raft::Cluster::becameLeader(node, term);
        leaderTimes.push_back(now());
    }

    std::vector<Time> electionTimes;  // elections started
    std::vector<Time> leaderTimes;    // elections won
    Time echoFrom = -1;               // print trace lines in [echoFrom, echoTo]
    Time echoTo = -2;
};

inline int leaderOf(raft::Cluster& c, int servers)
{
    int best = 0;
    for (int id = 1; id <= servers; ++id) {
        const raft::Node& n = c.node(id);
        if (n.up() && n.role() == raft::Role::Leader &&
            (best == 0 || n.term() > c.node(best).term())) {
            best = id;  // a deposed leader may still think it leads: take the newest term
        }
    }
    return best;
}

// Turn a target word into server ids (empty: nothing to do right now).
inline std::vector<int> resolve(raft::Cluster& c, int servers, const std::string& t)
{
    std::vector<int> ids;
    const int leader = leaderOf(c, servers);
    for (int id = 1; id <= servers; ++id) {
        const bool up = c.node(id).up();
        if ((t == "all") || (t == "leader" && id == leader) ||
            (t == "down" && !up && ids.empty()) ||
            (t == "follower" && up && id != leader && ids.empty()) ||
            (t == std::to_string(id))) {
            ids.push_back(id);
        }
    }
    return ids;
}

inline void apply(raft::Cluster& c, int servers, const Step& st, bool verbose)
{
    std::vector<int> ids;
    for (const std::string& a : st.args) {
        for (int id : resolve(c, servers, a)) {
            const bool up = c.node(id).up();
            if ((st.verb != "crash" || up) && (st.verb != "restart" || !up)) {
                ids.push_back(id);  // crash only running servers, restart only crashed ones
            }
        }
    }
    if (verbose) {
        std::printf("%5lld ms  *** fault: %s", static_cast<long long>(c.now()), st.verb.c_str());
        for (int id : ids) {
            std::printf(" n%d", id);
        }
        std::printf("%s\n", ids.empty() && st.verb != "heal" ? " (nothing to do now)" : "");
    }
    for (int id : ids) {
        if (st.verb == "crash") {
            c.crash(id);
        } else if (st.verb == "restart") {
            c.restart(id);
        } else if (st.verb == "isolate") {
            c.isolate(id);
        } else if (st.verb == "reconnect") {
            c.reconnect(id);
        }
    }
    if (st.verb == "partition" && !ids.empty()) {
        c.partition(ids);
    } else if (st.verb == "heal") {
        c.heal();
    }
}

// A generated schedule, in the style of DS302's fuzz.h, as steps of the same language.
inline std::vector<Step> nemesisSteps(const Scenario& s, std::uint64_t seed)
{
    std::vector<Step> out;
    sim::Rng plan(seed * 7919 + 1);  // its own generator: the schedule depends only on the seed
    for (Time t = s.nemFrom; !s.nemesis.empty() && t < s.nemTo; t += s.nemEvery) {
        const auto pick = [&] { return std::to_string(plan.range(1, s.servers)); };
        Step st;
        st.at = t;
        if (s.nemesis == "random") {
            const int what = static_cast<int>(plan.range(0, 5));
            const std::string a = pick();
            const std::string b = pick();
            const char* verbs[] = {"crash", "restart", "partition", "heal", "", ""};
            st.verb = verbs[what];
            st.args = what == 2 && a != b ? std::vector<std::string>{a, b}
                                          : std::vector<std::string>{a};
        } else {  // "hunter": go after whoever leads right now
            const int what = static_cast<int>(plan.range(0, 4));
            const char* verbs[] = {"crash", "restart", "isolate", "heal", "crash"};
            st.verb = verbs[what];
            st.args = {what == 1 ? "down" : (what == 4 ? pick() : "leader")};
        }
        if (!st.verb.empty()) {
            out.push_back(st);
        }
    }
    return out;
}

// ---- one run and its verdict --------------------------------------------------------

struct Result
{
    std::string scenario;
    std::uint64_t seed = 0;
    std::vector<std::string> violations;  // Raft safety (checked inside the run)
    bool linChecked = false;
    bool linearizable = false;
    bool live = false;                    // a probe write was answered after settle
    Time recovery = -1;                   // ms from settle to that answer
    int started = 0;                      // client operations (probes excluded)
    int answered = 0;
    Time p50 = 0;                         // simulated ms, answered operations only
    Time p99 = 0;
    int leaders = 0;
    int elections = 0;
    long dropped = 0;
    std::uint64_t fingerprint = 0;
    bool deterministic = true;
    std::vector<raft::HistoryOp> history;  // what the clients saw (probes included)

    bool pass() const
    {
        return violations.empty() && linChecked && linearizable && live && deterministic;
    }
    std::string why() const
    {
        if (!violations.empty()) {
            return violations.front();
        }
        if (!linChecked) {
            return "history too large for the checker";
        }
        if (!linearizable) {
            return "client history not linearizable";
        }
        if (!live) {
            return "no progress after the faults stopped";
        }
        return deterministic ? "ok" : "same seed gave a different run";
    }
};

inline void mix(std::uint64_t& h, std::int64_t v)
{
    for (int i = 0; i < 8; ++i) {  // FNV-1a over the eight bytes of v
        h ^= static_cast<std::uint64_t>(v >> (8 * i)) & 0xff;
        h *= 0x100000001b3ULL;
    }
}

inline Time percentile(std::vector<Time> v, int p)
{
    if (v.empty()) {
        return 0;
    }
    std::sort(v.begin(), v.end());
    const std::size_t rank = (v.size() * static_cast<std::size_t>(p) + 99) / 100;  // nearest rank
    return v[std::max<std::size_t>(rank, 1) - 1];
}

inline Result runOnce(const Scenario& s, std::uint64_t seed, const Variant& var = {},
                      Time echoFrom = -1, Time echoTo = -2)
{
    Observed c(s.servers, seed, var.options, var.faults, s.net);
    c.traceLevel = raft::Cluster::Trace::None;
    c.echoFrom = echoFrom;
    c.echoTo = echoTo;
    c.startClients(s.clients, s.maxOps);
    const bool verbose = echoFrom >= 0;

    std::vector<Step> steps = s.steps;
    for (const Step& st : nemesisSteps(s, seed)) {
        steps.push_back(st);
    }
    for (const Step& st : steps) {
        c.at(st.at, [&s, st, verbose](raft::Cluster& k) { apply(k, s.servers, st, verbose); });
    }
    c.at(s.settle, [&s, verbose](raft::Cluster& k) {
        apply(k, s.servers, Step{s.settle, "heal", {}}, verbose);
        apply(k, s.servers, Step{s.settle, "restart", {"all"}}, verbose);
    });

    // Liveness probes: every 100 ms after settle, until one is answered, write to the leader.
    std::vector<std::size_t> probes;
    for (Time t = s.settle + 100; t < s.end; t += 100) {
        c.at(t, [&s, &probes, t](raft::Cluster& k) {
            for (std::size_t i : probes) {
                if (k.history()[i].response >= 0) {
                    return;  // already live
                }
            }
            const int leader = leaderOf(k, s.servers);
            if (leader != 0) {
                k.submit(leader, raft::OpType::Put, 'x', static_cast<int>(1000 + t / 100));
                probes.push_back(k.history().size() - 1);
            }
        });
    }
    c.runUntil(s.end);

    Result r;
    r.scenario = s.name;
    r.seed = seed;
    r.violations = c.violations();
    lin::Checker checker(c.history());
    std::vector<int> order;
    r.linChecked = !checker.tooBig();
    r.linearizable = r.linChecked && checker.check(order);
    std::vector<Time> latency;
    std::uint64_t h = 0xcbf29ce484222325ULL;
    for (std::size_t i = 0; i < c.history().size(); ++i) {
        const raft::HistoryOp& op = c.history()[i];
        mix(h, op.process);
        mix(h, op.value);
        mix(h, op.invoke);
        mix(h, op.response);
        if (std::find(probes.begin(), probes.end(), i) != probes.end()) {
            if (op.response >= 0 && (r.recovery < 0 || op.response - s.settle < r.recovery)) {
                r.recovery = op.response - s.settle;
            }
            continue;
        }
        ++r.started;
        if (op.response >= 0) {
            ++r.answered;
            latency.push_back(op.response - op.invoke);
        }
    }
    for (int id = 1; id <= s.servers; ++id) {
        const raft::Node& n = c.node(id);
        mix(h, n.term());
        mix(h, n.commitIndex());
        for (const raft::Entry& e : n.log()) {
            mix(h, e.term * 1000003LL + e.value * 31LL + e.key);
        }
    }
    r.live = r.recovery >= 0;
    r.p50 = percentile(latency, 50);
    r.p99 = percentile(latency, 99);
    r.leaders = c.leaderCount();
    r.elections = static_cast<int>(c.electionTimes.size());
    r.dropped = c.dropped();
    r.fingerprint = h;
    r.history = c.history();
    return r;
}

// The harness's own test: run the same seed twice and compare fingerprints.
inline Result run(const Scenario& s, std::uint64_t seed, const Variant& var = {})
{
    Result first = runOnce(s, seed, var);
    const Result second = runOnce(s, seed, var);
    first.deterministic = first.fingerprint == second.fingerprint;
    return first;
}

} // namespace mp2
