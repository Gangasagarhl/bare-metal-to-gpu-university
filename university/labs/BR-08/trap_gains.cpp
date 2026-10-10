// BR-08 Listing 6, trap 1: trusting simulation gains on hardware.
// Part A (robot wheel): gains tuned for a fast response in the simulator, then run
// unchanged on the kit (the stand-in model), next to the cautious bring-up gains of F9-22.
// Part B (drone, one rate axis): a derivative gain that is harmless in a simulator without
// vibration, run in a model with vibration and a motor lag. Part B is a flight condition:
// it cannot be bench-tested with propellers removed, and here it is a model only.
#include "bench.hpp"
#include "kit_model.hpp"
#include <cmath>
#include <deque>
#include <format>
#include <iostream>
#include <string>

namespace {

struct LoopStats
{
    double peakRef = 0, holdStd = 0, saturated = 0;
};

LoopStats stats(const br08::Result& r)
{
    LoopStats s;
    int n = 0, sat = 0, nh = 0;
    double sum = 0, sum2 = 0;
    for (const br08::Row& w : r.rows) {
        const double t = w.tNominal;
        ++n;
        sat += std::abs(w.duty) >= 0.999 ? 1 : 0;
        if (t < 1.2) {
            s.peakRef = std::max(s.peakRef, w.reference);
        }
        if (t >= 0.6 && t < 1.2) { // the last 0.6 s of the 150 rad/s hold
            sum += w.reference;
            sum2 += w.reference * w.reference;
            ++nh;
        }
    }
    s.holdStd = std::sqrt(std::max(0.0, sum2 / nh - (sum / nh) * (sum / nh)));
    s.saturated = 100.0 * sat / n;
    return s;
}

br08::Result kitTrace; // the sim-tuned gains on the kit, printed after the table

void partA()
{
    std::cout << "Part A: robot wheel, 150 rad/s then 250 rad/s, ramp 3000 rad/s^2\n";
    // Simulator tuning (rule of F9-22, closed-loop time constant 0.008 s instead of 0.05 s)
    const br08::Gains fast{0.195 / (80.8 * 0.008), 0.195 / (80.8 * 0.008) / 0.195};
    std::cout << std::format("sim-tuned gains kp={:.4f} ki={:.4f}; bring-up gains kp={} ki={}\n",
                             fast.kp, fast.ki, br08::kSimGains.kp, br08::kSimGains.ki);
    std::cout << "gains      world      reference peak  overshoot  hold std  duty saturated\n";
    for (const auto& [name, g] :
         {std::pair{"sim-tuned", fast}, std::pair{"bring-up ", br08::kSimGains}}) {
        for (int world = 0; world < 2; ++world) {
            br08::BenchPlan plan;
            plan.benchSteps = false; // the bench steps passed earlier; only the loop is compared
            plan.rampPerSecond = 3000.0;
            plan.gains = g;
            br08::Result r;
            if (world == 0) {
                br08::SimWorld sim;
                r = br08::Bench(sim, plan).run();
            } else {
                br08::KitWorld kit{br08::KitParams{}};
                r = br08::Bench(kit, plan).run();
            }
            const LoopStats s = stats(r);
            if (world == 1 && g.kp > 0.1) {
                kitTrace = r;
            }
            // the kit's true speed is 1024/1000 times the measured one at steady state
            const double target = world == 0 ? 150.0 : 150.0 * 1024.0 / 1000.0;
            std::cout << std::format("{}  {:<9} {:>12.1f} {:>9.1f} % {:>8.2f} {:>10.1f} %  {}\n",
                                     name, world == 0 ? "simulator" : "kit", s.peakRef,
                                     100.0 * (s.peakRef - target) / target, s.holdStd,
                                     s.saturated, r.stop);
        }
    }
    std::cout << "\nkit, sim-tuned gains, 0.60 to 0.80 s (every 20 ms):\n"
                 "   t (s)  setpoint  measured  reference   duty\n";
    for (const br08::Row& w : kitTrace.rows) {
        const int ms = static_cast<int>(std::lround(w.tNominal * 1000));
        if (ms >= 600 && ms <= 800 && ms % 20 == 0) {
            std::cout << std::format("{:>8.2f} {:>9.1f} {:>9.1f} {:>10.1f} {:>6.2f}\n", w.tNominal,
                                     w.setpoint, w.measured, w.reference, w.duty);
        }
    }
}

// One rate axis of a multirotor: J dw/dt = torque; torque follows the command through a
// first-order motor lag. The gyro sees vibration at a fixed frequency plus noise.
// Invented teaching values; not a model of any vehicle.
struct AxisResult
{
    double errRms = 0, cmdStd = 0, saturated = 0;
};

AxisResult axis(double kp, double kd, double dTau, bool real)
{
    constexpr double dt = 0.0025;   // s, 400 Hz rate loop
    constexpr double inertia = 0.01; // kg m^2
    constexpr double torqueMax = 0.5; // N m
    const double lag = real ? 0.03 : 0.0; // s, motor and propeller spin-up (not in the sim)
    rb::PidConfig c;
    c.kp = kp;
    c.ki = 0.0;
    c.kd = kd;
    c.period = dt;
    c.outMin = -torqueMax;
    c.outMax = torqueMax;
    c.derivativeTau = dTau;
    rb::Pid pid = rb::Pid::create(c).value();
    br08::Rng rng(5);
    double w = 0, torque = 0, sumE2 = 0, sumC = 0, sumC2 = 0;
    int sat = 0;
    const int n = 1600; // 4 s
    for (int k = 0; k < n; ++k) {
        const double t = k * dt;
        const double target = (t >= 0.5 && t < 2.5) ? 1.0 : 0.0; // rad/s step and back
        double gyro = w;
        if (real) {
            gyro += 0.5 * std::sin(2 * 3.141592653589793 * 160.0 * t) + 0.02 * rng.gauss();
        }
        const double cmd = pid.update(target, gyro);
        sat += std::abs(cmd) >= torqueMax - 1e-9 ? 1 : 0;
        sumC += cmd;
        sumC2 += cmd * cmd;
        for (int s = 0; s < 10; ++s) { // 10 sub-steps per loop period
            torque += (lag > 0 ? (cmd - torque) * (dt / 10) / lag : cmd - torque);
            w += (dt / 10) * torque / inertia;
        }
        sumE2 += (target - w) * (target - w);
    }
    AxisResult r;
    r.errRms = std::sqrt(sumE2 / n);
    r.cmdStd = std::sqrt(std::max(0.0, sumC2 / n - (sumC / n) * (sumC / n)));
    r.saturated = 100.0 * sat / n;
    return r;
}

void partB()
{
    std::cout << "\nPart B: drone rate axis (model only; a flight condition, not a bench test)\n";
    std::cout << "gains                            world      rate error rms  command std"
                 "  saturated\n";
    struct G
    {
        const char* name;
        double kp, kd, dTau;
    };
    for (const G& g : {G{"sim-tuned kp 0.4 kd 0.002      ", 0.4, 0.002, 0.0},
                       G{"bring-up  kp 0.2 kd 0.0005 flt ", 0.2, 0.0005, 0.005}}) {
        for (bool real : {false, true}) {
            const AxisResult r = axis(g.kp, g.kd, g.dTau, real);
            std::cout << std::format("{} {:<9} {:>12.3f} {:>12.3f} {:>9.1f} %\n", g.name,
                                     real ? "flight" : "simulator", r.errRms, r.cmdStd,
                                     r.saturated);
        }
    }
}

} // namespace

int main()
{
    partA();
    partB();
    return 0;
}
