// gsdemo.cpp - a deterministic demonstration harness for the F12 worked example:
// a ground station that stores mission waypoints in a three-replica store (SE501, F12-30).
// This is the university's own teaching model, not Raft and not any real product.
// Reads a scenario from standard input, one event per line:
//   ack one|majority       when the store acknowledges a write
//   telemetry <seq>        a telemetry message from the (simulated) drone
//   write <key> <value>    the ground station stores a waypoint
//   replicate              deliver queued copies from the primary to live backups
//   crash <replica>        a replica stops; its queued outgoing copies are lost
//   failover               the live replica with the most writes becomes primary
//   read <key>             the ground station reads a waypoint back
//   check                  acceptance check A1: every acknowledged write is readable
// Exit code 0 when every check passes, 1 otherwise.
#include <array>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

struct Replica
{
    bool alive = true;
    long applied = 0;  // number of writes this replica holds
    std::map<std::string, std::string> data;
};

struct Write
{
    long seq;
    std::string key, value;
};

int main()
{
    std::array<Replica, 3> rep;
    int primary = 0;
    std::string ackPolicy = "majority";
    std::vector<Write> log;      // every write the primary accepted, in order
    std::vector<int> queuedFor;  // per queued copy: the backup it is for
    std::vector<long> queuedSeq;
    std::map<std::string, std::string> acked;  // what the ground station was told is stored
    long lastTelemetry = 0, gaps = 0;
    int tick = 0, failures = 0;

    auto apply = [&](Replica& r, const Write& w) {
        r.data[w.key] = w.value;
        r.applied = w.seq;
    };

    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream in(line);
        std::string ev;
        if (!(in >> ev) || ev[0] == '#') continue;
        ++tick;
        std::cout << "t" << tick << " ";
        if (ev == "ack") {
            in >> ackPolicy;
            std::cout << "store acknowledges after: " << ackPolicy << "\n";
        } else if (ev == "telemetry") {
            long seq = 0;
            in >> seq;
            if (lastTelemetry != 0 && seq != lastTelemetry + 1) {
                gaps += seq - lastTelemetry - 1;
                std::cout << "telemetry " << seq << " GAP of " << seq - lastTelemetry - 1 << "\n";
            } else {
                std::cout << "telemetry " << seq << " ok\n";
            }
            lastTelemetry = seq;
        } else if (ev == "write") {
            Write w{static_cast<long>(log.size()) + 1, "", ""};
            in >> w.key >> w.value;
            if (!rep[primary].alive) {
                std::cout << "write " << w.key << " FAIL (no primary)\n";
                continue;
            }
            int copies = 0;
            if (ackPolicy == "majority") {
                for (auto& r : rep) {
                    if (r.alive) ++copies;
                }
                if (copies < 2) {
                    std::cout << "write " << w.key << " REFUSED (only " << copies
                              << " of 3 replicas alive; no majority)\n";
                    continue;
                }
                for (auto& r : rep) {
                    if (r.alive) apply(r, w);
                }
            } else {
                apply(rep[primary], w);
                copies = 1;
                for (int b = 0; b < 3; ++b) {
                    if (b != primary && rep[b].alive) {
                        queuedFor.push_back(b);
                        queuedSeq.push_back(w.seq);
                    }
                }
            }
            log.push_back(w);
            acked[w.key] = w.value;
            std::cout << "write " << w.key << "=" << w.value << " ACK (stored on " << copies
                      << " of 3)\n";
        } else if (ev == "replicate") {
            std::size_t n = queuedFor.size();
            for (std::size_t i = 0; i < n; ++i) {
                if (rep[queuedFor[i]].alive) apply(rep[queuedFor[i]], log[queuedSeq[i] - 1]);
            }
            queuedFor.clear();
            queuedSeq.clear();
            std::cout << "replicate: " << n << " queued copies delivered\n";
        } else if (ev == "crash") {
            int r = 0;
            in >> r;
            rep[r].alive = false;
            std::cout << "replica " << r << " CRASHED";
            if (r == primary && !queuedFor.empty()) {
                std::cout << " (" << queuedFor.size()
                          << (queuedFor.size() == 1 ? " queued copy" : " queued copies")
                          << " lost with it)";
                queuedFor.clear();
                queuedSeq.clear();
            }
            std::cout << "\n";
        } else if (ev == "failover") {
            int best = -1;
            for (int r = 0; r < 3; ++r) {
                if (rep[r].alive && (best < 0 || rep[r].applied > rep[best].applied)) best = r;
            }
            if (best < 0) {
                std::cout << "failover FAILED (no live replica)\n";
                continue;
            }
            primary = best;
            std::cout << "failover: replica " << primary << " is primary (has the first "
                      << rep[primary].applied << " of " << log.size() << " writes)\n";
        } else if (ev == "read") {
            std::string key;
            in >> key;
            if (!rep[primary].alive) {
                std::cout << "read " << key << " FAIL (primary down; run failover)\n";
                continue;
            }
            auto& d = rep[primary].data;
            auto it = d.find(key);
            std::cout << "read " << key << " -> " << (it == d.end() ? "NOT FOUND" : it->second)
                      << " (from replica " << primary << ")\n";
        } else if (ev == "check") {
            std::vector<std::string> lost;
            for (const auto& [key, value] : acked) {
                auto& d = rep[primary].data;
                auto it = d.find(key);
                if (!rep[primary].alive || it == d.end() || it->second != value) {
                    lost.push_back(key + "=" + value);
                }
            }
            std::cout << "check A1 " << (lost.empty() ? "PASS" : "FAIL") << ": "
                      << acked.size() - lost.size() << " of " << acked.size()
                      << " acknowledged writes readable; telemetry gaps reported: " << gaps
                      << "\n";
            for (const auto& l : lost) {
                std::cout << "    lost: acknowledged " << l << " is not on the primary\n";
            }
            if (!lost.empty()) ++failures;
        } else {
            std::cout << "unknown event " << ev << "\n";
            ++failures;
        }
    }
    std::cout << "result: " << (failures == 0 ? "all checks pass" : "CHECK FAILED") << "\n";
    return failures == 0 ? 0 : 1;
}
