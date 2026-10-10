// gameday.cpp - F12-16 game-day simulator. A three-replica key-value service (a model of MP2)
// receives a bad configuration push at minute 6. The responders' actions come from stdin,
// one per line: "<minute> <actor> <verb> [details]". The program prints the minute-by-minute
// timeline, flags coordination problems, and sums the damage against the error budget.
// Optional argument: the last minute to show (a game master reveals the incident step by step).
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

struct Action
{
    int minute = 0;
    std::string actor, verb, rest;
};

struct Roles
{
    bool declared = false;
    int at = -1;
    std::string sev, ic, ops, comms, scribe;
};

// Value of "key=value" inside a details string, or "" if absent.
std::string field(const std::string& rest, const std::string& key)
{
    std::istringstream in(rest);
    std::string word;
    while (in >> word) {
        if (word.rfind(key + "=", 0) == 0) {
            return word.substr(key.size() + 1);
        }
    }
    return "";
}

int main(int argc, char** argv)
{
    const int kEnd = 32;               // minutes simulated
    const int kFault = 6;              // bad config v42 reaches every replica
    const int kRpm = 1000;             // client requests per minute (model)
    const double kBadFraction = 0.30;  // share of requests v42 makes fail
    const double kPageRatio = 0.02;    // page if the 5-minute error ratio is above 2 %
    const int kEscalate = 5;           // minutes before an unacknowledged page escalates
    const int kRestart = 3;            // a restarted replica is down for 3 minutes
    const int kRollout = 2;            // a rollback takes effect 2 minutes later
    // 30-day error budget for a 99.9 % success SLO at kRpm: 0.1 % of all requests.
    const double kBudget = 30.0 * 24 * 60 * kRpm * 0.001;
    int until = kEnd - 1;
    if (argc > 1) {
        until = std::stoi(argv[1]);
    }

    std::vector<Action> acts;
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        std::istringstream in(line);
        Action a;
        in >> a.minute >> a.actor >> a.verb;
        std::getline(in, a.rest);
        if (!a.rest.empty() && a.rest[0] == ' ') {
            a.rest.erase(0, 1);
        }
        acts.push_back(a);
    }

    std::string primary = "?", secondary = "?";
    int config = 41, rollback_at = -1, leader = 1;
    std::vector<int> down_until(4, -1);   // replica n is down while minute < down_until[n]
    std::vector<long> req(kEnd, 0), err(kEnd, 0);
    int paged = -1, escalated = -1, acked = -1, resolved = -1, last_update = -1, gap_flagged = -1;
    int last_change = -100, notes = 0, updates = 0;
    std::string last_changer;
    Roles roles;

    std::printf("minute  config  up      leader  requests  errors  ratio   | events\n");
    for (int t = 0; t < kEnd; ++t) {
        std::vector<std::string> ev;
        auto note = [&](const std::string& s) { ev.push_back("NOTE " + s); ++notes; };
        if (t == kFault) {
            config = 42;
            ev.push_back("deploy: config v42 pushed to n1 n2 n3 (the injected fault)");
        }
        if (t == rollback_at) {
            config = 41;
            ev.push_back("deploy: rollback to v41 complete");
        }
        for (const Action& a : acts) {
            if (a.minute != t) {
                continue;
            }
            const bool change = a.verb == "restart" || a.verb == "rollback";
            if (change) {
                if (!roles.declared) {
                    note("change by " + a.actor + " with no incident declared: nobody coordinates");
                } else if (a.actor != roles.ops) {
                    note("change by " + a.actor + ", but the ops lead is " + roles.ops);
                }
                if (t - last_change < 3 && a.actor != last_changer) {
                    note("concurrent changes by " + last_changer + " and " + a.actor);
                }
                last_change = t;
                last_changer = a.actor;
            }
            if (a.verb == "rota") {
                primary = field(a.rest, "primary");
                secondary = field(a.rest, "secondary");
            } else if (a.verb == "ack") {
                if (acked < 0 && paged >= 0) {
                    acked = t;
                }
                ev.push_back(a.actor + " acknowledges the page");
            } else if (a.verb == "declare") {
                roles = {true, t, a.rest.substr(0, a.rest.find(' ')), field(a.rest, "ic"),
                         field(a.rest, "ops"), field(a.rest, "comms"), field(a.rest, "scribe")};
                last_update = t;
                ev.push_back("INCIDENT " + roles.sev + " declared: IC " + roles.ic + ", ops " +
                             roles.ops + ", comms " + roles.comms + ", scribe " + roles.scribe);
                if (roles.ic == roles.ops) {
                    note("the IC also holds ops: nobody watches the whole incident");
                }
            } else if (a.verb == "restart") {
                const int n = a.rest.empty() ? 0 : a.rest.back() - '0';
                if (n >= 1 && n <= 3) {
                    down_until[n] = t + kRestart;
                    ev.push_back(a.actor + " restarts n" + std::to_string(n));
                }
            } else if (a.verb == "rollback") {
                rollback_at = t + kRollout;
                ev.push_back(a.actor + " starts rollback to v41");
            } else if (a.verb == "status") {
                last_update = t;
                ++updates;
                ev.push_back("status page (" + a.actor + "): " + a.rest);
            } else if (a.verb == "say") {
                ev.push_back("chat " + a.actor + ": " + a.rest);
            } else if (a.verb == "resolve") {
                resolved = t;
                ev.push_back(a.actor + " resolves the incident");
            }
        }

        // Replicas, quorum and leader election (an election costs the whole minute).
        std::string up;
        int n_up = 0;
        for (int n = 1; n <= 3; ++n) {
            if (t >= down_until[n]) {
                ++n_up;
                up += (up.empty() ? "" : ",") + std::to_string(n);
            }
        }
        bool election = false;
        if (t < down_until[leader]) {
            election = true;
            leader = 0;
            for (int n = 1; n <= 3 && n_up >= 2; ++n) {
                if (t >= down_until[n]) {
                    leader = n;
                    break;
                }
            }
        } else if (leader == 0 && n_up >= 2) {
            election = true;
            for (int n = 1; n <= 3; ++n) {
                if (t >= down_until[n]) {
                    leader = n;
                    break;
                }
            }
        }
        req[t] = kRpm;
        if (n_up < 2 || election || leader == 0) {
            err[t] = kRpm;
            ev.push_back(n_up < 2 ? "no quorum: every request fails" : "leader election");
        } else if (config == 42) {
            err[t] = static_cast<long>(kRpm * kBadFraction);
        }

        // Alerting: a fast-burn page on the 5-minute error ratio, then escalation.
        long wr = 0, we = 0;
        for (int k = std::max(0, t - 4); k <= t; ++k) {
            wr += req[k];
            we += err[k];
        }
        if (paged < 0 && static_cast<double>(we) / wr > kPageRatio) {
            paged = t;
            ev.push_back("PAGE to " + primary + " (primary): 5-min error ratio above 2 %");
        }
        if (paged >= 0 && acked < 0 && escalated < 0 && t - paged >= kEscalate) {
            escalated = t;
            ev.push_back("PAGE escalated to " + secondary + " (secondary): no acknowledgement");
        }
        if (roles.declared && resolved < 0 && t - last_update >= 15 && gap_flagged != last_update) {
            gap_flagged = last_update;
            note("no stakeholder update for 15 minutes");
        }
        if (resolved == t && err[t] > 0) {
            note("resolved while requests are still failing");
        }

        if (t <= until) {
            const std::string lead = leader == 0 ? "none" : "n" + std::to_string(leader);
            char buf[96];
            std::snprintf(buf, sizeof buf,
                          "t+%02d    v%d     %-7s %-6s  %5ld     %5ld  %5.1f %%  |", t, config,
                          up.c_str(), lead.c_str(), req[t], err[t], 100.0 * err[t] / req[t]);
            std::printf("%s", buf);
            if (ev.empty()) {
                std::printf("\n");
            }
            const std::string cont = std::string(58, ' ') + "|";  // continuation column
            for (std::size_t i = 0; i < ev.size(); ++i) {
                std::printf("%s %s\n", i == 0 ? "" : cont.c_str(), ev[i].c_str());
            }
        }
    }
    if (until < kEnd - 1) {
        std::printf("(game master: timeline shown up to t+%02d only)\n", until);
        return 0;
    }

    long bad = 0;
    int recovered = -1;
    for (int t = kFault; t < kEnd; ++t) {
        bad += err[t];
    }
    for (int t = kEnd - 1; t >= kFault && err[t] == 0; --t) {
        recovered = t;
    }
    auto when = [](int m) { return m < 0 ? std::string("never") : "t+" + std::to_string(m); };
    std::printf("\nsummary\n");
    std::printf("  fault injected            t+%d\n", kFault);
    std::printf("  first page                %s\n", when(paged).c_str());
    std::printf("  acknowledged              %s\n", when(acked).c_str());
    std::printf("  incident declared         %s\n", when(roles.declared ? roles.at : -1).c_str());
    std::printf("  errors stopped (for good) %s\n", when(recovered).c_str());
    std::printf("  resolved                  %s\n", when(resolved).c_str());
    std::printf("  failed requests           %ld\n", bad);
    std::printf("  30-day error budget used  %.1f %% (budget %.0f failed requests)\n",
                100.0 * bad / kBudget, kBudget);
    std::printf("  stakeholder updates       %d\n", updates);
    std::printf("  coordination notes        %d\n", notes);
    return 0;
}
