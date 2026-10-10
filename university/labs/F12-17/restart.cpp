// restart.cpp - F12-17 forensic evidence generator: "the restart that fixed it".
// An API service with two instances (api-1, api-2) calls a dependency, geo-lookup.
// From 19:05 to 19:49, 8 % of geo-lookup calls time out (the dependency's own fault).
// Evening traffic falls from 3000 to 1230 requests per minute. A responder restarts both
// API instances at 19:20 (each restart fails its instance's requests for that minute).
// The chat lines come from stdin (the scenario script) and are merged by time.
#include <cstdio>
#include <iostream>
#include <map>
#include <string>

int main()
{
    std::multimap<int, std::string> chat;  // minute after 19:00 -> line
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.size() > 6 && line[0] != '#') {
            const int minute = std::stoi(line.substr(3, 2));
            chat.emplace(minute, line);
        }
    }
    std::printf("time   requests  errors  error ratio  api-1 err  api-2 err  top error message\n");
    long seq = 0;
    for (int m = 0; m < 60; ++m) {
        const long req = 3000 - 30 * m;  // evening traffic decline (model)
        const bool dep_fault = m >= 5 && m < 50;
        const bool restart = m == 20;
        long err1 = 0, err2 = 0;
        const long half = req / 2;
        if (restart) {
            err1 = half;  // api-1 restarting: its share fails
            err2 = half;  // api-2 restarted in the same minute
        } else if (dep_fault) {
            err1 = half * 8 / 100;
            err2 = (req - half) * 8 / 100;
        }
        const long err = err1 + err2;
        const char* top = err == 0 ? "-"
                          : restart ? "connection refused (instance restarting)"
                                    : "upstream geo-lookup: timeout after 800 ms";
        std::printf("19:%02d  %8ld  %6ld  %9.1f %%  %9ld  %9ld  %s\n", m, req, err,
                    100.0 * err / req, err1, err2, top);
        for (auto it = chat.lower_bound(m); it != chat.upper_bound(m); ++it) {
            std::printf("       chat %s\n", it->second.c_str());
        }
        if (m == 12 || m == 33) {
            for (int k = 0; k < 3; ++k) {
                ++seq;
                std::printf("       log  19:%02d:%02d api-%d req=%06ld GET /v1/route status=503 "
                            "upstream=geo-lookup elapsed_ms=8%02d\n",
                            m, 7 + 13 * k, k % 2 + 1, 41000 + 977 * seq + m, k * 3 + 1);
            }
        }
    }
    return 0;
}
