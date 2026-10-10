// lostlink_gen.cpp - F10-31 forensic evidence generator ("Lost link"). It replays a 50 s
// flight on paper: who sent a heartbeat when, and which of them arrived. Events (exercise
// values, explained only in the answer key): a radio fade in both directions, a period in
// which the ground-station laptop's program stops, and a second ground-station app started
// on a phone. It prints the two logs the team recovered, one row per second. SYNTHETIC
// evidence from our own program, not a real flight.
#include <cstdio>
#include <string>
#include <vector>

namespace {

struct Hb {
    double t;     // send time, s
    int seq;
};

std::vector<Hb> sender(double phase, int seqStep, double start, double stop,
                       double pauseFrom = -1, double pauseTo = -1)
{
    std::vector<Hb> out;
    int seq = 0;
    for (double t = start + phase; t < stop; t += 1.0) {
        if (t >= pauseFrom && t < pauseTo) {
            continue;                                   // nothing sent: seq does not advance
        }
        out.push_back({t, seq % 256});
        seq += seqStep;
    }
    return out;
}

bool radioFade(double t) { return t >= 12.4 && t < 15.9; }
bool laptopFrozen(double t) { return t >= 27.6 && t < 36.6; }

std::string cell(const std::vector<Hb>& hbs, int second, bool (*lost)(double))
{
    std::string s;
    for (const Hb& h : hbs) {
        if (h.t >= second && h.t < second + 1 && !lost(h.t)) {
            char buf[32];
            std::snprintf(buf, sizeof buf, "%s%.2f#%d", s.empty() ? "" : " ", h.t, h.seq);
            s += buf;
        }
    }
    return s.empty() ? "-" : s;
}

bool noLoss(double) { return false; }
bool laptopRxLoss(double t) { return radioFade(t) || laptopFrozen(t); }

}  // namespace

int main()
{
    // vehicle 1/1: heartbeat at k+0.30 s; 5 other messages between heartbeats -> seq step 6
    const auto vehicle = sender(0.30, 6, 0, 50);
    // laptop GCS 255/190: heartbeat at k+0.10 s, only heartbeats -> seq step 1; frozen 27.6-36.6
    const auto laptop = sender(0.10, 1, 0, 50, 27.6, 36.6);
    // phone app 255/1: started at 20 s, heartbeat at k+0.75 s
    const auto phone = sender(0.75, 1, 20, 50);

    std::printf("# LOG A: ground-station laptop, vehicle heartbeats received (time s#seq)\n");
    std::printf("# LOG B: vehicle onboard log, ground heartbeats received, by sender sys/comp\n");
    std::printf("# vehicle settings: ground-station failsafe ENABLED, timeout 5.0 s, action RTL\n");
    std::printf("# vehicle mode log: AUTO from 2.0 s to 50.0 s; no failsafe event recorded\n");
    std::printf("sec | A: from 1/1        | B: from 255/190    | B: from 255/1\n");
    for (int s = 0; s < 50; ++s) {
        std::printf("%3d | %-18s | %-18s | %s\n", s, cell(vehicle, s, laptopRxLoss).c_str(),
                    cell(laptop, s, radioFade).c_str(), cell(phone, s, noLoss).c_str());
    }
    return 0;
}
