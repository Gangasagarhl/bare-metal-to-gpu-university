// F9-20 Listing 1: the tuning test bench. A cart (m = 2 kg, c = 0.8 N s/m) on a slight
// slope that pushes it back with a constant 1.5 N, a PID position controller running
// every 10 ms, and the measurements that the written specification asks for.
#pragma once
#include <algorithm>
#include <cmath>
#include <vector>

struct Gains
{
    double kp; // N per m
    double ki; // N per (m s)
    double kd; // N s per m
};

struct Metrics
{
    double riseTime;     // s, from 10 % to 90 % of the step
    double overshoot;    // % of the step
    double settlingTime; // s, last time outside +-2 % of the step
    double finalError;   // m, largest |error| during the last 2 s
    double peakAsked;    // N, largest |force| the controller asked for
    double iae;          // m s, sum of |error| * period
};

// If trace is given, the position is appended to it every 0.1 s (for plots).
inline Metrics simulate(const Gains& g, double runTime = 10.0, std::vector<double>* trace = nullptr)
{
    const double period = 0.01;
    const double mass = 2.0;
    const double friction = 0.8;
    const double slope = -1.5;   // N, always pushing back
    const double setpoint = 1.0; // m
    double x = 0.0;
    double v = 0.0;
    double integral = 0.0;
    double previousX = 0.0;
    double t10 = -1.0;
    double t90 = -1.0;
    double peak = 0.0;
    Metrics m{0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    const int steps = static_cast<int>(std::lround(runTime / period));
    for (int k = 0; k < steps; ++k) {
        const double t = k * period;
        if (trace != nullptr && k % 10 == 0) {
            trace->push_back(x);
        }
        const double error = setpoint - x;
        integral += error * period;
        const double asked = g.kp * error + g.ki * integral - g.kd * (x - previousX) / period;
        previousX = x;
        const double force = std::clamp(asked, -10.0, 10.0);
        m.peakAsked = std::max(m.peakAsked, std::abs(asked));
        m.iae += std::abs(error) * period;
        if (t10 < 0.0 && x >= 0.1) {
            t10 = t;
        }
        if (t90 < 0.0 && x >= 0.9) {
            t90 = t;
        }
        peak = std::max(peak, x);
        if (std::abs(error) > 0.02) {
            m.settlingTime = t + period;
        }
        if (t >= runTime - 2.0) {
            m.finalError = std::max(m.finalError, std::abs(error));
        }
        for (int s = 0; s < 100; ++s) {
            const double dt = period / 100;
            v += dt * (force + slope - friction * v) / mass;
            x += dt * v;
        }
    }
    m.riseTime = (t10 >= 0.0 && t90 >= 0.0) ? t90 - t10 : -1.0;
    m.overshoot = 100.0 * std::max(0.0, peak - setpoint) / setpoint;
    return m;
}

// The written specification of the practical exam (one step of 1 m):
//   S1 overshoot <= 5 %   S2 settling (2 %) <= 3.0 s   S3 final error <= 2 mm
//   S4 the controller never asks for more than 10 N (so the motor never saturates)
inline bool meetsSpec(const Metrics& m)
{
    return m.overshoot <= 5.0 && m.settlingTime <= 3.0 && m.finalError <= 0.002 &&
           m.peakAsked <= 10.0;
}
