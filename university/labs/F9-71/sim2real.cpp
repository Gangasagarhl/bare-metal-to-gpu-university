// sim2real.cpp - F9-71: the same PI speed controller on a "simulated" motor and on motors with
// the effects simulators often leave out: actuation delay, friction, encoder quantisation,
// measurement noise, a sagging battery, and a wrong encoder parameter.
// Input (stdin), one case per line:
//   case NAME DELAY_MS FRICTION NOISE VBAT CPR_TRUE CPR_ASSUMED
// Every case runs 1.5 s with a step to 60 rad/s at t = 0.1 s; the controller and its gains are
// identical in every case. All "robots" here are models in this program: they stand in for
// hardware the build container does not have, and they are only as true as their equations.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <deque>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

struct Case {
    std::string name;
    int delay_ms = 0;          // the command reaches the motor this many ms late
    double friction = 0;       // Coulomb friction, as a deceleration in rad/s^2
    double noise = 0;          // +- this many encoder counts of jitter per sample
    double vbat = 12;          // supply voltage: the command saturates at +- vbat
    int cpr_true = 1024;       // encoder counts per revolution, as built (0: perfect sensor)
    int cpr_assumed = 1024;    // counts per revolution, as written in the software's parameters
};

struct Result {
    double overshoot_pct, settle_ms, true_mean, measured_mean, sat_pct;
};

Result run(const Case& c, bool print_trace)
{
    const double dt = 0.001, tau = 0.05, gain = 10.0;   // motor: d(w)/dt = (gain*u - w)/tau
    const double kp = 0.4, ki = 8.0, target = 60.0;     // PI gains tuned on the ideal model
    double w = 0, angle = 0, integ = 0;
    int64_t last_count = 0;
    std::deque<double> pipe(static_cast<size_t>(c.delay_ms), 0.0);
    uint32_t rng = 12345;
    double peak = 0, settle = -1, true_sum = 0, meas_sum = 0;
    int n_avg = 0, saturated = 0, steps = 1500;
    for (int k = 0; k < steps; ++k) {
        double t = k * dt;
        double ref = t >= 0.1 ? target : 0.0;
        // sensor: encoder counts, with jitter, converted with the ASSUMED counts per revolution
        rng = rng * 1664525u + 1013904223u;
        double jitter = c.noise * ((rng >> 8) / 8388608.0 - 1.0);
        double measured = w;                            // CPR 0: a perfect speed sensor
        if (c.cpr_true > 0) {
            auto count = static_cast<int64_t>(std::floor(angle / (2 * M_PI) * c.cpr_true + jitter));
            measured = static_cast<double>(count - last_count) * 2 * M_PI / c.cpr_assumed / dt;
            last_count = count;
        }
        // controller: PI with clamping anti-windup
        double err = ref - measured;
        double u = kp * err + ki * integ;
        double u_sat = std::clamp(u, -c.vbat, c.vbat);
        if (u == u_sat) {
            integ += err * dt;
        } else {
            ++saturated;
        }
        // actuator: the command arrives delay_ms later
        pipe.push_back(u_sat);
        double applied = pipe.front();
        pipe.pop_front();
        // plant, integrated in 10 sub-steps
        for (int s = 0; s < 10; ++s) {
            double h = dt / 10;
            double fr = w > 0.01 ? -c.friction : (w < -0.01 ? c.friction : 0.0);
            w += h * ((gain * applied - w) / tau + fr);
            angle += h * w;
        }
        if (t >= 0.1) {
            peak = std::max(peak, w);
            if (std::abs(w - target) > 0.05 * target) {
                settle = -1;
            } else if (settle < 0) {
                settle = (t - 0.1) * 1000;
            }
        }
        if (t >= 1.3) {
            true_sum += w;
            meas_sum += measured;
            ++n_avg;
        }
        if (print_trace && k % 100 == 0) {
            std::cout << "    t=" << std::setw(4) << k << " ms  command " << std::setw(7) << std::fixed
                      << std::setprecision(2) << u_sat << " V  measured " << std::setw(7) << measured
                      << "  true " << std::setw(7) << w << " rad/s\n";
        }
    }
    return {std::max(0.0, (peak - target) / target * 100), settle, true_sum / n_avg, meas_sum / n_avg,
            100.0 * saturated / steps};
}

int main()
{
    std::string line;
    std::cout << std::left << std::setw(16) << "case" << std::right << std::setw(10) << "overshoot"
              << std::setw(10) << "settle" << std::setw(12) << "true w" << std::setw(12) << "measured w"
              << std::setw(11) << "saturated" << '\n';
    while (std::getline(std::cin, line)) {
        std::istringstream in(line);
        std::string kind;
        Case c;
        in >> kind;
        if (kind == "trace") {
            in >> c.name >> c.delay_ms >> c.friction >> c.noise >> c.vbat >> c.cpr_true >> c.cpr_assumed;
            std::cout << "trace of " << c.name << ":\n";
            run(c, true);
            continue;
        }
        if (kind != "case") {
            continue;
        }
        in >> c.name >> c.delay_ms >> c.friction >> c.noise >> c.vbat >> c.cpr_true >> c.cpr_assumed;
        Result r = run(c, false);
        std::ostringstream settle;
        if (r.settle_ms < 0) {
            settle << "never";
        } else {
            settle << std::fixed << std::setprecision(0) << r.settle_ms << " ms";
        }
        std::cout << std::left << std::setw(16) << c.name << std::right << std::fixed << std::setprecision(1)
                  << std::setw(9) << r.overshoot_pct << '%' << std::setw(10) << settle.str() << std::setw(12)
                  << r.true_mean << std::setw(12) << r.measured_mean << std::setw(10) << r.sat_pct << "%\n";
    }
    std::cout << "(target 60.0 rad/s; true w and measured w are means over t = 1.3..1.5 s; settle = within 5 %)\n";
    return 0;
}
