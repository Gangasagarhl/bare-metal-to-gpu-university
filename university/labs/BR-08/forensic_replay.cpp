// BR-08 forensic answer key: replay the afternoon one change at a time.
// Starts from the morning kit and adds, alone and together, the three things that differed
// in the afternoon (lower battery; firmware 1.4's longer command delay; firmware 1.4's late
// samples), then tries the fix: speed computed from the measured sample interval.
#include "bench.hpp"
#include "kit_model.hpp"
#include <cmath>
#include <format>
#include <iostream>

namespace {

void run(const char* label, bool battery, bool delay, bool bursts, bool stampFix)
{
    br08::KitParams p;
    p.seed = 22;
    if (battery) {
        p.batteryOpen = 10.6;
    }
    if (delay) {
        p.commandDelay = 0.030;
    }
    if (bursts) {
        p.burstEvery = 10;
        p.burstLate = 0.004;
    }
    br08::BenchPlan plan;
    plan.signDuty = 0.2;
    plan.gains = {0.1207, 0.6188};
    plan.timestampSpeed = stampFix;
    br08::KitWorld kit{p};
    const br08::Result r = br08::Bench(kit, plan).run();
    double loop0 = -1, peakRef = 0, m = 0, m2 = 0, f = 0, f2 = 0;
    int n = 0;
    for (const br08::Row& w : r.rows) {
        if (w.phase != "loop") {
            continue;
        }
        if (loop0 < 0) {
            loop0 = w.tNominal;
        }
        const double tl = w.tNominal - loop0;
        if (tl < 1.2) {
            peakRef = std::max(peakRef, w.reference);
        }
        if (tl >= 0.6 && tl < 1.2) {
            m += w.measured;
            m2 += w.measured * w.measured;
            f += w.reference;
            f2 += w.reference * w.reference;
            ++n;
        }
    }
    auto sd = [n](double a, double a2) { return std::sqrt(std::max(0.0, a2 / n - a * a / n / n)); };
    std::cout << std::format("{:<40} {:>8.1f} {:>9.2f} {:>9.2f}\n", label, peakRef, sd(m, m2),
                             sd(f, f2));
}

} // namespace

int main()
{
    std::cout << "run (seed 22 throughout)                  peak ref  meas std   ref std\n";
    run("morning kit", false, false, false, false);
    run("+ battery 10.6 V only", true, false, false, false);
    run("+ command delay 30 ms only", false, true, false, false);
    run("+ late samples only", false, false, true, false);
    run("+ both firmware effects", false, true, true, false);
    run("+ everything (the afternoon)", true, true, true, false);
    run("afternoon, speed from measured interval", true, true, true, true);
    run("afternoon fix, with firmware 1.3 timing", true, false, false, true);
    return 0;
}
