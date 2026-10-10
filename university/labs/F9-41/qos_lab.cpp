// qos_lab.cpp - the university's model of DDS/ROS 2 quality of service (F9-41).
// Implements the "requested versus offered" rule for four policies, a late-joiner experiment
// for durability and history, and a deadline check. Policy names follow the DDS specification's
// names as the chapter records them (pending verification); the printed report format is ours.
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

enum class Reliability { BestEffort = 0, Reliable = 1 };
enum class Durability { Volatile = 0, TransientLocal = 1 };
enum class Liveliness { Automatic = 0, ManualByTopic = 1 };

struct Qos {
    Reliability rel = Reliability::Reliable;
    Durability dur = Durability::Volatile;
    int depth = 10;                 // history KEEP_LAST(depth)
    int deadlineMs = 0;             // 0 = infinite
    Liveliness live = Liveliness::Automatic;
};

const char* name(Reliability r) { return r == Reliability::Reliable ? "RELIABLE" : "BEST_EFFORT"; }
const char* name(Durability d) { return d == Durability::TransientLocal ? "TRANSIENT_LOCAL" : "VOLATILE"; }
const char* name(Liveliness l) { return l == Liveliness::ManualByTopic ? "MANUAL_BY_TOPIC" : "AUTOMATIC"; }

// Requested-versus-offered: the offer must be at least as strong as the request.
std::vector<std::string> incompatibilities(const Qos& offered, const Qos& requested)
{
    std::vector<std::string> why;
    if (offered.rel < requested.rel)
        why.push_back(std::string("reliability: offered ") + name(offered.rel) + " < requested " + name(requested.rel));
    if (offered.dur < requested.dur)
        why.push_back(std::string("durability: offered ") + name(offered.dur) + " < requested " + name(requested.dur));
    bool offInf = offered.deadlineMs == 0, reqInf = requested.deadlineMs == 0;
    if (!reqInf && (offInf || offered.deadlineMs > requested.deadlineMs))
        why.push_back("deadline: offered period " + (offInf ? std::string("infinite") : std::to_string(offered.deadlineMs) + " ms") +
                      " > requested " + std::to_string(requested.deadlineMs) + " ms");
    if (offered.live < requested.live)
        why.push_back(std::string("liveliness: offered ") + name(offered.live) + " < requested " + name(requested.live));
    return why;                     // history depth never makes endpoints incompatible
}

Qos parse(std::istringstream& in)
{
    Qos q; std::string kv;
    while (in >> kv) {
        auto eq = kv.find('=');
        std::string k = kv.substr(0, eq), v = kv.substr(eq + 1);
        if (k == "reliability") q.rel = (v == "reliable") ? Reliability::Reliable : Reliability::BestEffort;
        else if (k == "durability") q.dur = (v == "transient_local") ? Durability::TransientLocal : Durability::Volatile;
        else if (k == "depth") q.depth = std::stoi(v);
        else if (k == "deadline") q.deadlineMs = std::stoi(v);
        else if (k == "liveliness") q.live = (v == "manual_by_topic") ? Liveliness::ManualByTopic : Liveliness::Automatic;
    }
    return q;
}

std::string describe(const Qos& q)
{
    return std::string("reliability=") + name(q.rel) + " durability=" + name(q.dur) + " history=KEEP_LAST(" +
           std::to_string(q.depth) + ") deadline=" + (q.deadlineMs ? std::to_string(q.deadlineMs) + "ms" : "infinite") +
           " liveliness=" + name(q.live);
}

struct Endpoint { std::string node; Qos qos; };

void topicInfo(const std::string& topic, const std::string& type, const std::vector<Endpoint>& pubs,
               const std::vector<Endpoint>& subs, bool verdict)
{
    std::printf("topic %s  type %s\n  publishers: %zu\n", topic.c_str(), type.c_str(), pubs.size());
    for (const auto& p : pubs) std::printf("    node %-12s %s\n", p.node.c_str(), describe(p.qos).c_str());
    std::printf("  subscriptions: %zu\n", subs.size());
    for (const auto& s : subs) std::printf("    node %-12s %s\n", s.node.c_str(), describe(s.qos).c_str());
    if (!verdict) return;                  // "endpoints": the evidence only, no conclusion
    for (const auto& p : pubs)
        for (const auto& s : subs) {
            auto why = incompatibilities(p.qos, s.qos);
            std::printf("  match %s -> %s: %s\n", p.node.c_str(), s.node.c_str(), why.empty() ? "compatible" : "INCOMPATIBLE");
            for (const auto& w : why) std::printf("      %s\n", w.c_str());
        }
}

void matrix()
{
    std::printf("reliability: offered (rows) x requested (columns)\n%-13s %-13s %-13s\n", "", "BEST_EFFORT", "RELIABLE");
    for (int o = 0; o < 2; ++o) {
        std::printf("%-13s", name(static_cast<Reliability>(o)));
        for (int r = 0; r < 2; ++r) {
            Qos off, req; off.rel = static_cast<Reliability>(o); req.rel = static_cast<Reliability>(r);
            std::printf(" %-13s", incompatibilities(off, req).empty() ? "ok" : "incompatible");
        }
        std::printf("\n");
    }
    std::printf("durability: offered (rows) x requested (columns)\n%-16s %-16s %-16s\n", "", "VOLATILE", "TRANSIENT_LOCAL");
    for (int o = 0; o < 2; ++o) {
        std::printf("%-16s", name(static_cast<Durability>(o)));
        for (int r = 0; r < 2; ++r) {
            Qos off, req; off.dur = static_cast<Durability>(o); req.dur = static_cast<Durability>(r);
            std::printf(" %-16s", incompatibilities(off, req).empty() ? "ok" : "incompatible");
        }
        std::printf("\n");
    }
}

// Late joiner: the publisher sent samples 1..sent before the subscriber matched.
void lateJoin(const Qos& pub, const Qos& sub, int sent)
{
    std::printf("late joiner: publisher %s depth %d sent %d samples before the subscriber %s depth %d appeared -> ",
                name(pub.dur), pub.depth, sent, name(sub.dur), sub.depth);
    if (!incompatibilities(pub, sub).empty()) { std::printf("not matched, receives nothing\n"); return; }
    if (pub.dur == Durability::TransientLocal && sub.dur == Durability::TransientLocal) {
        int keep = std::min(std::min(pub.depth, sent), sub.depth);   // writer history, then reader depth
        std::printf("receives %d old sample(s): %d..%d\n", keep, sent - keep + 1, sent);
    } else {
        std::printf("receives no old samples, only new ones\n");
    }
}

// Deadline: the publisher publishes every periodMs; the subscription expects one every deadlineMs.
void deadline(int periodMs, int deadlineMs, int durationMs)
{
    int missed = 0;
    for (int t = periodMs; t <= durationMs; t += periodMs) missed += (periodMs - 1) / deadlineMs;
    std::printf("deadline: publishing every %d ms, requested deadline %d ms, %d ms -> %d deadline-missed event(s)\n",
                periodMs, deadlineMs, durationMs, missed);
}

int main()
{
    std::string line, topic, type;
    std::vector<Endpoint> pubs, subs;
    while (std::getline(std::cin, line)) {
        std::istringstream in(line);
        std::string cmd; in >> cmd;
        if (cmd == "matrix") matrix();
        else if (cmd == "topic") { in >> topic >> type; pubs.clear(); subs.clear(); }
        else if (cmd == "pub" || cmd == "sub") {
            Endpoint e; in >> e.node; e.qos = parse(in);
            (cmd == "pub" ? pubs : subs).push_back(e);
        }
        else if (cmd == "info") topicInfo(topic, type, pubs, subs, true);
        else if (cmd == "endpoints") topicInfo(topic, type, pubs, subs, false);
        else if (cmd == "latejoin") {
            int sent; in >> sent;
            lateJoin(pubs.at(0).qos, subs.at(0).qos, sent);
        }
        else if (cmd == "deadline") { int p, d, t; in >> p >> d >> t; deadline(p, d, t); }
        else if (cmd == "#" || cmd.empty()) continue;
    }
    return 0;
}
