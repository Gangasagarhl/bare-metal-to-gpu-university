// F11-16 forensic evidence generator: "Who sent that command?"
// A quadcopter in a simulated survey flight lands on its own at t = 46.3 s. Its telemetry
// link was unsigned. This program simulates both ends and writes the two logs the team
// handed over: rx_log.csv (every frame the drone's receiver accepted) and gcs_tx.csv
// (every frame the ground-station app sent). It prints the window 42-50 s of both.
// SYNTHETIC: ids, levels, rates and times are exercise values; the files are this run's.
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

struct Rx
{
    int tMs;
    int rssi;
    int sysid, compid, seq;
    std::string msg;
};

int main()
{
    std::uint32_t s = 20261010;
    auto noise = [&s]() {
        s = s * 1664525u + 1013904223u;
        return static_cast<int>((s >> 24) % 5) - 2;    // -2..+2
    };
    std::vector<Rx> rx;
    std::vector<std::pair<int, std::string>> tx;    // time, "seq msg"
    int seq = 0;
    for (int t = 0; t <= 60000; t += 500) {
        // ground station: heartbeat every 1000 ms, position request every 500 ms
        std::vector<std::string> msgs;
        if (t % 1000 == 0) {
            msgs.push_back("heartbeat");
        }
        msgs.push_back("request:position");
        if (t == 2000) {
            msgs.push_back("cmd:arm");
        }
        if (t == 3000) {
            msgs.push_back("cmd:takeoff");
        }
        if (t == 5000) {
            msgs.push_back("mission:start");
        }
        int dt = 0;
        for (const std::string& m : msgs) {
            const int at = t + 7 + dt;
            dt += 3;
            tx.push_back({at, std::to_string(seq) + " " + m});
            const int distance = t / 1000;                       // drone flies away slowly
            rx.push_back({at + 9, -46 - distance / 6 + noise(), 20, 190, seq, m});
            seq = (seq + 1) & 255;
        }
        // the other transmitter: copies the ground station's ids, guesses the next seq
        if (t == 46000) {
            rx.push_back({46288, -79 + noise(), 20, 190, seq, "cmd:land"});
        }
        if (t == 46500) {
            rx.push_back({46791, -80 + noise(), 20, 190, seq, "cmd:land"});
        }
    }
    // the receiver logs in arrival order
    for (std::size_t i = 1; i < rx.size(); ++i) {
        for (std::size_t j = i; j > 0 && rx[j - 1].tMs > rx[j].tMs; --j) {
            std::swap(rx[j - 1], rx[j]);
        }
    }
    std::FILE* f = std::fopen("rx_log.csv", "w");
    std::FILE* g = std::fopen("gcs_tx.csv", "w");
    if (f == nullptr || g == nullptr) {
        std::printf("cannot write the logs\n");
        return 1;
    }
    std::fprintf(f, "t_ms,rssi_dbm,sysid,compid,seq,msg\n");
    for (const Rx& r : rx) {
        std::fprintf(f, "%d,%d,%d,%d,%d,%s\n", r.tMs, r.rssi, r.sysid, r.compid, r.seq,
                     r.msg.c_str());
    }
    std::fprintf(g, "t_ms,seq,msg\n");
    for (const auto& [t, m] : tx) {
        const std::size_t sp = m.find(' ');
        std::fprintf(g, "%d,%s,%s\n", t, m.substr(0, sp).c_str(), m.substr(sp + 1).c_str());
    }
    std::fclose(f);
    std::fclose(g);
    std::printf("rx_log.csv: %zu frames; gcs_tx.csv: %zu frames\n", rx.size(), tx.size());
    std::printf("--- rx_log.csv, 42000 <= t_ms < 50000 ---\nt_ms,rssi_dbm,sysid,compid,seq,msg\n");
    for (const Rx& r : rx) {
        if (r.tMs >= 42000 && r.tMs < 50000) {
            std::printf("%d,%d,%d,%d,%d,%s\n", r.tMs, r.rssi, r.sysid, r.compid, r.seq,
                        r.msg.c_str());
        }
    }
    std::printf("--- gcs_tx.csv, 42000 <= t_ms < 50000 ---\nt_ms,seq,msg\n");
    for (const auto& [t, m] : tx) {
        if (t >= 42000 && t < 50000) {
            const std::size_t sp = m.find(' ');
            std::printf("%d,%s,%s\n", t, m.substr(0, sp).c_str(), m.substr(sp + 1).c_str());
        }
    }
    return 0;
}
