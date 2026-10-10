// BR-08 Listing 3: the paired run. The same controller, the same bench sequence and the
// same log format, once in the simulator and once on the kit (here: the stand-in model).
// Writes sim_run.csv and kit_run.csv and prints a short side-by-side summary.
#include "bench.hpp"
#include "kit_model.hpp"
#include <cmath>
#include <format>
#include <fstream>
#include <iostream>
#include <string>

namespace {

struct Summary
{
    double restNoise = 0, stepFinal = 0, loopMean150 = 0, loopRef150 = 0, peak150 = 0,
           voltsMin = 1e9, intervalMax = 0;
};

Summary summarise(const br08::Result& r)
{
    Summary s;
    double sum = 0, sum2 = 0, prev = -1, stepStart = -1, loopStart = -1;
    int n = 0, nf = 0, nl = 0;
    for (const br08::Row& w : r.rows) {
        if (w.phase == "step" && stepStart < 0) {
            stepStart = w.tNominal;
        }
        if (w.phase == "loop" && loopStart < 0) {
            loopStart = w.tNominal;
        }
        if (prev >= 0) {
            s.intervalMax = std::max(s.intervalMax, w.tSample - prev);
        }
        prev = w.tSample;
        s.voltsMin = std::min(s.voltsMin, w.volts);
        if (w.phase == "rest") {
            sum += w.measured;
            sum2 += w.measured * w.measured;
            ++n;
        }
        // the last 0.2 s of the step test
        if (w.phase == "step" && w.tNominal - stepStart >= 0.8 - 1e-9) {
            s.stepFinal += w.measured;
            ++nf;
        }
        if (w.phase == "loop") {
            const double tl = w.tNominal - loopStart;
            if (tl < 1.2) {
                s.peak150 = std::max(s.peak150, w.measured);
            }
            if (tl >= 0.8 - 1e-9 && tl < 1.2 - 1e-9) { // last 0.4 s of the 150 rad/s hold
                s.loopMean150 += w.measured;
                s.loopRef150 += w.reference;
                ++nl;
            }
        }
    }
    s.restNoise = n ? std::sqrt(std::max(0.0, sum2 / n - (sum / n) * (sum / n))) : 0.0;
    s.stepFinal /= nf ? nf : 1;
    s.loopMean150 /= nl ? nl : 1;
    s.loopRef150 /= nl ? nl : 1;
    return s;
}

br08::Result runAndLog(br08::LoggedMotorIo& io, const br08::BenchPlan& plan,
                       const std::string& file, const std::string& conditions)
{
    br08::Bench bench(io, plan);
    const br08::Result r = bench.run();
    std::ofstream out(file);
    br08::writeLog(out, io, plan, r, conditions);
    std::cout << std::format("{:<16} {} rows (stop: {})\n", file, r.rows.size(), r.stop);
    return r;
}

} // namespace

int main()
{
    // First attempt: the plan exactly as it passed in the simulator (F9-22).
    // (The simulator passed this plan in F9-22, run R3: sign test +25.6 rad/s.)
    br08::BenchPlan plan;
    br08::KitWorld kit0{br08::KitParams{}};
    const br08::Result first = runAndLog(kit0, plan, "kit_attempt1.csv",
                                         "bench, wheel off the ground, e-stop tested, seed 8");
    std::cout << "kit attempt 1, last rows of the sign test (t_sample, measured, duty, volts):\n";
    for (std::size_t i = first.rows.size() - 3; i < first.rows.size(); ++i) {
        const br08::Row& w = first.rows[i];
        std::cout << std::format("  {:.4f} {:>6.2f} {:.2f} {:.2f}\n", w.tSample, w.measured,
                                 w.duty, w.volts);
    }

    // One change, written down before the run: the sign-test duty, 0.1 -> 0.2.
    // Both worlds run the changed plan again, so the pair stays a pair.
    plan.signDuty = 0.2;
    std::cout << "\nchange: sign-test duty 0.1 -> 0.2 (nothing else)\n";
    br08::SimWorld sim;
    const br08::Result a =
        runAndLog(sim, plan, "sim_run.csv", "ideal clock, 12 V constant supply, no delay");
    br08::KitWorld kit{br08::KitParams{}};
    const br08::Result b = runAndLog(kit, plan, "kit_run.csv",
                                     "bench, wheel off the ground, e-stop tested, seed 8");

    const Summary s = summarise(a);
    const Summary k = summarise(b);
    std::cout << "\nquantity (from the two logs)                  simulator      kit\n";
    std::cout << std::format("speed noise at rest, rad/s (std)            {:>10.2f} {:>8.2f}\n",
                             s.restNoise, k.restNoise);
    std::cout << std::format("largest sample interval, ms                 {:>10.2f} {:>8.2f}\n",
                             1000 * s.intervalMax, 1000 * k.intervalMax);
    std::cout << std::format("lowest supply voltage, V                    {:>10.2f} {:>8.2f}\n",
                             s.voltsMin, k.voltsMin);
    std::cout << std::format("step test (duty 0.25) final speed, rad/s     {:>10.1f} {:>8.1f}\n",
                             s.stepFinal, k.stepFinal);
    std::cout << std::format("loop at 150 rad/s: peak measured, rad/s      {:>10.1f} {:>8.1f}\n",
                             s.peak150, k.peak150);
    std::cout << std::format("loop at 150 rad/s: mean measured, rad/s      {:>10.2f} {:>8.2f}\n",
                             s.loopMean150, k.loopMean150);
    std::cout << std::format("loop at 150 rad/s: mean reference, rad/s     {:>10.2f} {:>8.2f}\n",
                             s.loopRef150, k.loopRef150);
    return 0;
}
