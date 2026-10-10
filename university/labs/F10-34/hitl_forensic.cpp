// hitl_forensic.cpp - evidence generator for the F10-34 forensic lab.
// A HITL bench where the PC-to-flight-controller direction is a serial link of fixed
// capacity. The flight controller runs on its own 10 ms clock and always uses the newest
// COMPLETE sensor frame it has received. At t = 20 s an extra 60-byte frame per step is
// added to the same link (the scenario's "debug stream"). The vehicle holds position at
// 10 m in gusty wind; "hold err" is the largest horizontal distance from the hold point.
// Everything is modelled with the course simulator; the link capacity is a setting of this
// model, not of a real product.
#include "../F10-33/dronesim.hpp"

#include <cmath>
#include <cstdio>
#include <deque>

struct Queued
{
    double bytes_left;
    bool sensor;
    dn::Sensors s;
};

int main()
{
    const double capacity = 115200.0 / 10.0;  // bytes/s: a 115,200 bit/s setting, 10 bits per byte
    const double sensor_frame = 58, debug_frame = 60;
    dn::World w;
    dn::Fsw f;
    dn::Sensors s = w.sense();
    f.arm(s, {});
    std::deque<Queued> link;
    dn::Sensors fc_view = s;  // newest complete sensor frame on the FC side
    double queued_bytes = 0, offered = 0, max_age = 0, max_err = 0;
    bool debug = false;
    int k = 0;
    std::printf("bench log: link %.0f bytes/s; columns are per 2 s window\n", capacity);
    std::printf("%6s %12s %10s %15s %12s\n", "t[s]", "offered B/s", "queue[B]", "sensor age[ms]",
                "hold err [m]");
    while (w.t < 90.0 && !w.crashed) {
        s = w.sense();
        link.push_back({sensor_frame, true, s});
        offered += sensor_frame;
        if (debug) {
            link.push_back({debug_frame, false, {}});
            offered += debug_frame;
        }
        double budget = capacity * dn::kDt;  // bytes the link moves in one 10 ms step
        while (budget > 0 && !link.empty()) {
            const double n = std::min(budget, link.front().bytes_left);
            link.front().bytes_left -= n;
            budget -= n;
            if (link.front().bytes_left <= 0) {
                if (link.front().sensor) fc_view = link.front().s;
                link.pop_front();
            }
        }
        queued_bytes = 0;
        for (const auto& q : link) queued_bytes += q.bytes_left;
        w.wind = {2.0 * std::sin(0.4 * w.t), 1.0 * std::cos(0.3 * w.t), 0};  // gusty wind, m/s
        const dn::Command c = f.step(fc_view);  // the FC's own clock: one step every 10 ms
        w.step(c);
        if (!debug && w.t >= 20.0) {
            debug = true;
            std::printf("-- 20.00 s debug stream enabled\n");
        }
        max_age = std::max(max_age, (w.t - fc_view.t) * 1000.0);
        if (w.t > 6.0) max_err = std::max(max_err, dn::hnorm(w.pos - f.sp));
        if (++k % 200 == 0) {
            std::printf("%6.1f %12.0f %10.0f %15.0f %12.2f\n", w.t, offered / 2.0, queued_bytes,
                        max_age, max_err);
            offered = 0;
            max_age = 0;
            max_err = 0;
        }
    }
    if (w.crashed) std::printf("crashed at %.2f s\n", w.t);
    return 0;
}
