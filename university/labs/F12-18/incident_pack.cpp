// incident_pack.cpp - F12-18 Listing 1: generates the full incident pack of the course
// forensic lab from a model of the MP2 key-value service (three replicas, kv-1 leader).
// The fault: a nightly compaction job on kv-1 copies data before deleting it, fills the
// disk, fails, and leaves its temporary file behind. Writes need the leader's write-ahead
// log (WAL), so every write fails while kv-1 leads with a full disk; kv-1 still sends
// heartbeats, so no follower takes over. Human actions and chat lines come from stdin:
// "!HH:MM <action> <args>" for actions, "HH:MM name: text" for chat.
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <map>
#include <string>
#include <vector>

std::string hhmm(int m)  // minutes after midnight -> "HH:MM"
{
    char buf[8];
    std::snprintf(buf, sizeof buf, "%02d:%02d", (m / 60) % 24, m % 60);
    return buf;
}

int at(const std::string& s)  // "HH:MM" -> minutes after midnight
{
    return std::stoi(s.substr(0, 2)) * 60 + std::stoi(s.substr(3, 2));
}

int main()
{
    std::multimap<int, std::string> chat, actions;
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.size() > 6 && line[0] == '!') {
            actions.emplace(at(line.substr(1, 5)), line.substr(7));
        } else if (line.size() > 6 && line[0] != '#') {
            chat.emplace(at(line.substr(0, 5)), line.substr(6));
        }
    }
    auto acted = [&](int m, const std::string& what) {
        for (auto it = actions.lower_bound(m); it != actions.upper_bound(m); ++it) {
            if (it->second.rfind(what, 0) == 0) {
                return true;
            }
        }
        return false;
    };

    const int kStart = at("01:58"), kEnd = at("03:08");
    const int kWrites = 600, kReads = 2400;  // requests per minute (model)
    double disk = 78.0;                      // kv-1 disk use in %, after the retention change
    bool compacting = false, temp_left = false;
    int leader = 1, kv1_down_until = -1, paged = -1, ticket = -1, cleared = -1;
    std::vector<std::string> alerts, logs;
    std::vector<long> werr;
    std::string prev;  // the last printed row's state, to print only changes
    std::printf("=== METRICS (requests per minute; a row every 5 minutes and at every change) "
                "===\n");
    std::printf("time   kv-1 disk  leader  writes  write err  reads  read err\n");
    for (int m = kStart; m < kEnd; ++m) {
        const std::string t = hhmm(m);
        if (m == at("02:00")) {
            compacting = true;
            logs.push_back(t + ":00 kv-1 compaction: start (writes new segment file before "
                               "deleting old ones)");
        }
        if (compacting) {
            disk = std::min(100.0, disk + 1.5);
            if (disk >= 100.0) {
                compacting = false;
                temp_left = true;
                logs.push_back(t + ":02 kv-1 wal.append: error ENOSPC (no space left on device)");
                logs.push_back(t + ":09 kv-1 compaction: failed: ENOSPC; temp file "
                                   "compact-0412.tmp not removed");
            }
        }
        if (acted(m, "restart kv-1")) {
            kv1_down_until = m + 3;
            logs.push_back(t + ":05 kv-1 shutdown requested (signal from operator)");
        }
        if (acted(m, "delete-temp kv-1") && temp_left) {
            temp_left = false;
            disk = 80.0;
            logs.push_back(t + ":30 kv-1 compact-0412.tmp deleted by operator; disk 80 %");
        }
        bool election = false;
        if (m < kv1_down_until && leader == 1) {
            leader = 2;
            election = true;
            logs.push_back(t + ":06 kv-2 election timeout; term 19 -> 20; elected leader (votes "
                               "kv-2, kv-3)");
        }
        if (m == kv1_down_until) {
            logs.push_back(t + ":12 kv-1 started; joined as follower of kv-2 (term 20)");
        }
        const bool kv1_full = disk >= 100.0;
        if (leader == 2 && kv1_full && m >= kv1_down_until && m % 10 == 0) {
            logs.push_back(t + ":15 kv-2 replicate to kv-1: rejected (follower append ENOSPC); "
                               "kv-1 lagging");
        }
        if (leader == 2 && !kv1_full && m >= kv1_down_until && kv1_down_until > 0 &&
            acted(m, "delete-temp kv-1")) {
            logs.push_back(t + ":48 kv-2 replicate to kv-1: caught up");
        }
        long we = 0;
        if (election || (leader == 1 && kv1_full)) {
            we = kWrites;
        }
        werr.push_back(we);
        const std::string state = std::to_string(leader) + "/" + std::to_string(we) + "/" +
                                  std::to_string(disk >= 95.0) + std::to_string(disk >= 100.0);
        if (m % 5 == 0 || state != prev) {
            std::printf("%s  %7.1f %%  kv-%d   %6d  %9ld  %5d  %8d\n", t.c_str(), disk,
                        leader, kWrites, we, kReads, 0);
        }
        prev = state;

        if (ticket < 0 && disk >= 95.0) {
            ticket = m;
            alerts.push_back(t + " TICKET  kv-1 disk above 95 % (queue: storage-tickets, "
                                 "business hours)");
        }
        long w = 0;
        int n = 0;
        for (int k = static_cast<int>(werr.size()) - 1; k >= 0 && n < 5; --k, ++n) {
            w += werr[static_cast<std::size_t>(k)];
        }
        const double ratio = static_cast<double>(w) / (kWrites * n);
        if (paged < 0 && ratio > 0.02) {
            paged = m;
            alerts.push_back(t + " PAGE    kv write error ratio above 2 % over 5 min (primary "
                                 "on-call: rui)");
        }
        if (paged >= 0 && cleared < 0 && ratio == 0.0) {
            cleared = m;
            alerts.push_back(t + " CLEARED kv write error ratio back to 0 % over 5 min");
        }
        for (auto it = actions.lower_bound(m); it != actions.upper_bound(m); ++it) {
            if (it->second.rfind("ack", 0) == 0) {
                alerts.push_back(t + " ACK     page acknowledged by " + it->second.substr(4));
            }
        }
    }

    std::printf("\n=== ALERTS ===\n");
    for (const std::string& a : alerts) {
        std::printf("%s\n", a.c_str());
    }
    std::printf("\n=== LOGS (kv-1, kv-2; selected lines, in time order) ===\n");
    for (const std::string& l : logs) {
        std::printf("%s\n", l.c_str());
    }
    std::printf("\n=== TRACES (three sampled requests) ===\n");
    std::printf(
        "02:18:22 trace 7f3a  PUT /kv/user:1832  client -> kv-2 (follower) -> leader kv-1\n"
        "                     kv-1 wal.append 2 ms ERROR ENOSPC -> response 503 after 9 ms\n"
        "02:18:22 trace 7f3b  GET /kv/user:1832  client -> kv-3 (follower read) 3 ms -> 200\n"
        "02:41:09 trace 91c0  PUT /kv/user:1832  client -> kv-2 (leader) wal.append 4 ms,\n"
        "                     replicate kv-3 ok, kv-1 rejected -> committed (2 of 3) -> 200\n");
    std::printf("\n=== CHAT (#kv-incident; the scenario script, merged by time) ===\n");
    for (const auto& [m, text] : chat) {
        std::printf("%s %s\n", hhmm(m).c_str(), text.c_str());
    }
    std::printf("\n=== CHANGE LOG (last 7 days) ===\n");
    std::printf("3 days before  kv config: wal retention 4 -> 8 segments (reviewed; disk 60 %% "
                "-> 78 %%)\n");
    std::printf("daily 02:00    kv-1 compaction job (unchanged for 5 months)\n");
    return 0;
}
