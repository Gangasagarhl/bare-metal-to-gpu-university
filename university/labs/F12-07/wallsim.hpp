// wallsim.hpp - the university's tiny simulated world for F12-07: a robot on a straight
// corridor drives toward a shelf. The range sensor can be given a latency (the reading is
// that old when the controller gets it) and a uniform noise. Everything is deterministic:
// the noise comes from std::mt19937 with a fixed seed, turned into a number by our own code
// (the standard fixes mt19937's output sequence, but not the algorithm of its distributions).
#pragma once
#include "../F12-06/speedctl.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <deque>
#include <random>

namespace wallsim {

struct World {
    double start_d = 4.0;        // m, distance from the sensor to the shelf at t = 0
    double latency_s = 0.0;      // age of each reading when the controller receives it
    double noise_m = 0.0;        // half-width of uniform noise added to each reading
    double accel = 2.0;          // m/s^2, how fast the motor can change speed
    double duration_s = 20.0;
    std::uint32_t seed = 1;
};

struct Reading {
    double stamp_s;  // when the sensor measured
    double value_m;  // what it measured
};

// The controller under test sees only readings and returns a speed command.
// compensate = true is the fix of the forensic lab: predict where the robot is now.
struct Controller {
    bool compensate = false;
    double last_cmd = 0.0;
    double command(const Reading& r, double now_s)
    {
        double d = r.value_m;
        if (compensate) {
            d -= last_cmd * (now_s - r.stamp_s);  // distance driven since the reading
        }
        last_cmd = speedctl::governor(d);
        return last_cmd;
    }
};

struct Tick {  // one control period, as a robot would log it
    double t, stamp, reading, cmd, speed, true_d;
};

struct Outcome {
    double min_d = 1e9;
    double final_d = 0.0;
    double final_v = 0.0;
    bool contact = false;
};

constexpr double kDt = 0.01;      // physics step, s
constexpr int kControlEvery = 5;  // control period 0.05 s

// Ctrl is anything with double command(const Reading&, double now): the in-process Controller
// above, or a proxy that sends the reading to a controller in another process (plant_main.cc).
template <class Ctrl, class OnTick>
Outcome run(const World& w, Ctrl c, OnTick&& on_tick)
{
    std::mt19937 rng(w.seed);
    Outcome o;
    double d = w.start_d, v = 0.0, cmd = 0.0;
    const int lag = static_cast<int>(std::lround(w.latency_s / kDt));
    std::deque<Reading> pipe;  // readings travelling from the sensor to the controller
    const int n = static_cast<int>(std::lround(w.duration_s / kDt));
    for (int i = 0; i < n; ++i) {
        const double t = i * kDt;
        const double u = static_cast<double>(rng() % 2001) / 1000.0 - 1.0;  // -1 .. +1
        pipe.push_back({t, d + u * w.noise_m});
        if (static_cast<int>(pipe.size()) > lag + 1) {
            pipe.pop_front();
        }
        if (i % kControlEvery == 0) {
            const Reading r = pipe.front();  // lag steps old
            cmd = c.command(r, t);
            on_tick(Tick{t, r.stamp_s, r.value_m, cmd, v, d});
        }
        v += std::clamp(cmd - v, -w.accel * kDt, w.accel * kDt);
        d -= v * kDt;
        o.min_d = std::min(o.min_d, d);
        if (d <= 0.0) {
            o.contact = true;
            break;
        }
    }
    o.final_d = d;
    o.final_v = v;
    return o;
}

// Requirement S1 of F12-06: never closer than 0.35 m. S2: stopped within 0.65 m at the end.
inline bool meets_requirements(const Outcome& o)
{
    return !o.contact && o.min_d >= 0.35 && o.final_v < 0.02 && o.final_d <= 0.65;
}

}  // namespace wallsim
