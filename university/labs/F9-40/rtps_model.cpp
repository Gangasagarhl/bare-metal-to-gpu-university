// rtps_model.cpp - the university's model of what DDS/RTPS does underneath ROS 2 (F9-40).
// Not a DDS implementation and not the RTPS wire format: it models discovery inside a domain,
// endpoint matching by topic and type, sequence numbers, a writer's history (KEEP_LAST depth),
// and reliable repair with HEARTBEAT / ACKNACK / GAP over a channel that loses chosen samples.
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

struct Participant { std::string name; int domain = 0; };
struct Endpoint { std::string owner, topic, type; bool reliable = false; int depth = 0; };

struct Reader {
    std::map<long, bool> got;              // sequence number -> received
    std::vector<long> delivered;           // in the order handed to the application
    long nextExpected = 1;
    std::set<long> gapped;                 // announced as never coming (GAP)
};

void deliverInOrder(Reader& r, bool reliable)
{
    if (!reliable) return;                 // best effort delivers on arrival (done by caller)
    while (r.got.count(r.nextExpected) || r.gapped.count(r.nextExpected)) {
        if (r.got.count(r.nextExpected)) r.delivered.push_back(r.nextExpected);
        ++r.nextExpected;
    }
}

void run(const std::string& title, const std::vector<Participant>& parts, const Endpoint& w,
         const Endpoint& rd, const std::set<long>& drops, long count)
{
    std::printf("== scenario %s\n", title.c_str());
    // 1. participant discovery: each participant announces itself to its own domain only
    std::map<std::string, std::set<std::string>> knows;
    for (const auto& a : parts)
        for (const auto& b : parts)
            if (a.name != b.name && a.domain == b.domain) knows[a.name].insert(b.name);
    for (const auto& p : parts) {
        std::printf("  discovery: participant %-7s domain %d knows:", p.name.c_str(), p.domain);
        if (knows[p.name].empty()) std::printf(" (nobody)");
        for (const auto& n : knows[p.name]) std::printf(" %s", n.c_str());
        std::printf("\n");
    }
    // 2. endpoint discovery and matching: same topic, same type, compatible reliability
    bool sameDomain = knows[w.owner].count(rd.owner) > 0;
    bool match = sameDomain && w.topic == rd.topic && w.type == rd.type && (w.reliable || !rd.reliable);
    std::printf("  writer %s/%s [%s] %s depth %d; reader %s/%s [%s] %s -> %s\n", w.owner.c_str(), w.topic.c_str(),
                w.type.c_str(), w.reliable ? "reliable" : "best-effort", w.depth, rd.owner.c_str(), rd.topic.c_str(),
                rd.type.c_str(), rd.reliable ? "reliable" : "best-effort", match ? "MATCHED" : "not matched");
    // 3. data: the writer numbers samples 1..count and keeps the newest 'depth' in its history
    Reader r;
    std::vector<long> history;
    long sentPackets = 0, resent = 0;
    auto heartbeat = [&](long seq) {
        if (!match || !w.reliable || !rd.reliable) return;
        long first = history.front(), last = history.back();
        std::vector<long> missing;
        for (long s = r.nextExpected; s <= last; ++s)
            if (!r.got.count(s) && !r.gapped.count(s)) missing.push_back(s);
        std::printf("  t%-3ld HEARTBEAT first=%ld last=%ld -> ACKNACK missing:", seq, first, last);
        if (missing.empty()) std::printf(" none");
        for (long s : missing) std::printf(" %ld", s);
        std::printf("\n");
        for (long s : missing) {
            if (std::find(history.begin(), history.end(), s) != history.end()) {
                r.got[s] = true; ++resent; ++sentPackets;
                std::printf("       resend DATA seq=%ld (still in history)\n", s);
            } else {
                r.gapped.insert(s);
                std::printf("       GAP seq=%ld (no longer in history: overwritten)\n", s);
            }
        }
        deliverInOrder(r, true);
    };
    for (long seq = 1; seq <= count; ++seq) {
        history.push_back(seq);
        if (static_cast<int>(history.size()) > w.depth) history.erase(history.begin());
        if (!match) continue;
        ++sentPackets;
        if (drops.count(seq)) {
            std::printf("  t%-3ld DATA seq=%ld lost on the network\n", seq, seq);
        } else {
            r.got[seq] = true;
            if (!(w.reliable && rd.reliable)) r.delivered.push_back(seq);
            deliverInOrder(r, w.reliable && rd.reliable);
        }
        if (seq % 4 == 0 || seq == count) heartbeat(seq);
    }
    std::printf("  application received %zu of %ld:", r.delivered.size(), count);
    for (long s : r.delivered) std::printf(" %ld", s);
    std::printf("\n  packets sent %ld (of which resent %ld); permanently lost:", sentPackets, resent);
    bool any = false;
    for (long s = 1; s <= count; ++s)
        if (std::find(r.delivered.begin(), r.delivered.end(), s) == r.delivered.end()) { std::printf(" %ld", s); any = true; }
    std::printf("%s\n", any ? "" : " none");
}

int main()
{
    std::string word, title;
    std::vector<Participant> parts;
    Endpoint w, rd;
    std::set<long> drops;
    while (std::cin >> word) {
        if (word == "scenario") { std::cin >> title; parts.clear(); drops.clear(); }
        else if (word == "participant") { Participant p; std::cin >> p.name >> p.domain; parts.push_back(p); }
        else if (word == "writer" || word == "reader") {
            Endpoint e; std::string rel;
            std::cin >> e.owner >> e.topic >> e.type >> rel;
            e.reliable = (rel == "reliable");
            if (word == "writer") { std::cin >> e.depth; w = e; } else rd = e;
        }
        else if (word == "drop") { long s; while (std::cin >> s && s > 0) drops.insert(s); }   // list ends with 0
        else if (word == "send") { long n; std::cin >> n; run(title, parts, w, rd, drops, n); }
    }
    return 0;
}
