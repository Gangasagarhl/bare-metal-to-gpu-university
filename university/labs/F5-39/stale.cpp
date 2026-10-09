// stale.cpp - generates the evidence pack of the F5-39 forensic lab "the report that went back
// in time". A GFS-style file system (one master, chunk servers, clients with a location cache)
// runs a scripted scenario; each component writes its own log. The fault that was injected is
// described only in the chapter's answer key. Clocks are synchronised in this scenario.
#include <algorithm>
#include <cstdio>
#include <map>
#include <string>
#include <vector>

struct Clock
{
    int ms = 0;
    std::string str() const
    {
        char b[32];
        std::snprintf(b, sizeof b, "10:%02d:%02d.%03d", 14 + ms / 60000, (ms / 1000) % 60, ms % 1000);
        return b;
    }
};

int main()
{
    Clock c;
    std::map<std::string, std::vector<std::string>> logs;
    auto at = [&](int ms, const std::string& who, const std::string& line) {
        c.ms = ms;
        logs[who].push_back(c.str() + " " + line);
    };

    // 10:14:02 the report builder opens the file and caches the chunk locations
    at(2000, "client-report", "open /reports/daily: master says chunk 7731 v4 at cs2,cs5,cs7");
    at(2001, "client-report", "location cache: chunk 7731 -> cs2,cs5,cs7 (v4), ttl 120 s");
    at(2010, "client-report", "read chunk 7731 from cs5 (same rack): total=1712");
    at(2011, "cs5", "read chunk 7731 v4 offset 0 len 4096 for client-report");

    // cs5 keeps serving its own rack but its heartbeats stop reaching the master
    for (int t = 40000; t <= 100000; t += 10000) {
        at(t, "cs5", "heartbeat to master: last 10 sends failed (no route), retrying every 1 s");
    }
    at(41000, "master", "cs5: no heartbeat for 11000 ms (limit 10000): marked dead");
    at(41002, "master", "chunk 7731: replicas cs2,cs7 (1 below target 3): re-replicate cs2 -> cs8");
    at(41300, "cs8", "received chunk 7731 v4 from cs2 (clone)");

    // 10:14:50 the nightly job appends the corrected totals
    at(50000, "client-writer", "append to /reports/daily: master grants lease");
    at(50001, "master", "chunk 7731: lease to cs2, version 4 -> 5, replicas cs2,cs7,cs8");
    at(50003, "cs2", "chunk 7731 now v5 (lease granted)");
    at(50003, "cs7", "chunk 7731 now v5 (lease granted)");
    at(50004, "cs8", "chunk 7731 now v5 (lease granted)");
    at(50020, "cs2", "append chunk 7731 v5 offset 4096 len 512 (primary): ok, forwarded to cs7,cs8");
    at(50024, "cs7", "append chunk 7731 v5 offset 4096 len 512: ok");
    at(50025, "cs8", "append chunk 7731 v5 offset 4096 len 512: ok");
    at(50031, "client-writer", "append ok: total=1840 (corrected) written at offset 4096");

    // 10:15:05 the report builder runs again, with its cached locations
    at(65000, "client-report", "open /reports/daily: location cache hit for chunk 7731 (age 63 s)");
    at(65001, "client-report", "read chunk 7731 from cs5 (same rack)");
    at(65003, "cs5", "read chunk 7731 v4 offset 0 len 8192 for client-report: returned 4096 bytes (end of chunk)");
    at(65010, "client-report", "report built: total=1712");
    at(65011, "client-report", "report published to dashboard");

    // 10:15:40 the network path is repaired
    at(100500, "cs5", "heartbeat to master: ok; reporting chunks 7731 v4, 7802 v2, 7810 v1");
    at(100502, "master", "cs5 reports chunk 7731 v4 < current v5: stale replica, delete");
    at(100503, "master", "cs5 back: chunks 7802, 7810 current");
    at(100520, "cs5", "deleted chunk 7731 (master: stale)");
    at(130000, "client-report", "open /reports/daily: cache expired, master says chunk 7731 v5 at cs2,cs7,cs8");
    at(130010, "client-report", "read chunk 7731 from cs7: total=1840 (corrected)");

    std::printf("=== configuration (excerpt) ===\n");
    std::printf("replication_target = 3\nheartbeat_interval_ms = 1000\ndead_after_ms = 10000\n");
    std::printf("client_location_cache_ttl_s = 120\nclient_read_preference = same_rack_first\n");
    std::printf("chunkserver_check_lease_before_read = false\n\n");
    for (const auto& [who, lines] : logs) {
        std::printf("=== %s.log ===\n", who.c_str());
        std::vector<std::string> sorted = lines;   // each log in time order
        std::stable_sort(sorted.begin(), sorted.end());
        for (const auto& l : sorted) {
            std::printf("%s\n", l.c_str());
        }
        std::printf("\n");
    }
    return 0;
}
