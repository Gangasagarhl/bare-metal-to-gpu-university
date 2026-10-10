// incident.h - the evidence pack of the MP2 forensic lab, produced by a real run.
// Five servers and no injected faults. A front end sends one request every 200 ms
// (a quiet afternoon) to the server it believes is the leader. The report prints
// what an on-call engineer would have: the deployed options, a slice of the log,
// a metrics timeline in 500 ms windows, the SLO result and the checkers' verdict.
#include <cstdio>
#include <vector>

#include "harness.h"

inline void incidentReport(const char* title, const raft::Options& opt, std::uint64_t seed)
{
    const mp2::Time end = 4000;
    const mp2::Time window = 500;
    mp2::Observed c(5, seed, opt, raft::Faults{}, sim::NetOptions{});
    c.traceLevel = raft::Cluster::Trace::None;
    std::vector<int> termAt;          // highest term of any server at the end of each window
    std::vector<mp2::Time> noLeader;  // requests the front end could not send anywhere
    for (mp2::Time t = window; t <= end; t += window) {
        c.at(t, [&termAt](raft::Cluster& k) {
            int top = 0;
            for (int id = 1; id <= 5; ++id) {
                top = std::max(top, k.node(id).term());
            }
            termAt.push_back(top);
        });
    }
    int value = 0;
    for (mp2::Time t = 400; t < end - 200; t += 200) {
        c.at(t, [&noLeader, &value, t](raft::Cluster& k) {
            const int leader = mp2::leaderOf(k, 5);
            if (leader == 0) {
                noLeader.push_back(t);
            } else if (++value % 2 == 1) {
                k.submit(leader, raft::OpType::Put, 'x', value);
            } else {
                k.submit(leader, raft::OpType::Get, 'x');
            }
        });
    }
    c.echoFrom = 2000;  // the log slice, as shipped to the log store
    c.echoTo = 2400;
    std::printf("== %s ==\n", title);
    std::printf("deployed options: electionMin %lld ms, electionMax %lld ms, heartbeat %lld ms, "
                "maxBatch %d, noopOnElection %s\n", static_cast<long long>(opt.electionMin),
                static_cast<long long>(opt.electionMax), static_cast<long long>(opt.heartbeat),
                opt.maxBatch, opt.noopOnElection ? "on" : "off");
    std::printf("network: delay 2-10 ms, no loss; injected faults: none; "
                "load: 1 request per 200 ms from 400 ms\n");
    std::printf("-- log slice (all servers, 2000-2400 ms) --\n");
    c.runUntil(end);

    std::printf("-- metrics, 500 ms windows (simulated time) --\n");
    std::printf("window        elections  leaders  max term  sent  answered  no leader  "
                "p99 latency\n");
    int good = 0;
    int total = static_cast<int>(noLeader.size());
    for (mp2::Time w = 0; w < end; w += window) {
        const auto in = [w, window](mp2::Time t) { return t > w && t <= w + window; };
        int el = 0;
        int won = 0;
        int sent = 0;
        int lost = 0;
        for (mp2::Time t : c.electionTimes) {
            el += in(t);
        }
        for (mp2::Time t : c.leaderTimes) {
            won += in(t);
        }
        for (mp2::Time t : noLeader) {
            lost += in(t);
        }
        std::vector<mp2::Time> lat;
        for (const raft::HistoryOp& h : c.history()) {
            sent += in(h.invoke);
            if (h.response >= 0 && in(h.response)) {
                lat.push_back(h.response - h.invoke);
            }
        }
        std::printf("%4lld-%4lld ms  %9d  %7d  %8d  %4d  %8zu  %9d  %8lld ms\n",
                    static_cast<long long>(w), static_cast<long long>(w + window), el, won,
                    termAt[static_cast<std::size_t>(w / window)], sent, lat.size(), lost,
                    static_cast<long long>(mp2::percentile(lat, 99)));
    }
    for (const raft::HistoryOp& h : c.history()) {
        ++total;
        good += h.response >= 0 && h.response - h.invoke <= 150;
    }
    std::printf("-- SLO: 95 %% of requests answered within 150 ms --\n");
    std::printf("good %d of %d = %.1f %% -> %s\n", good, total, 100.0 * good / total,
                good * 100 >= total * 95 ? "met" : "MISSED");
    lin::Checker checker(c.history());
    std::vector<int> order;
    std::printf("-- checkers --\nsafety: %s; linearizable: %s\n",
                c.violations().empty() ? "no violation" : c.violations().front().c_str(),
                checker.tooBig() ? "too big" : (checker.check(order) ? "yes" : "NO"));
}
