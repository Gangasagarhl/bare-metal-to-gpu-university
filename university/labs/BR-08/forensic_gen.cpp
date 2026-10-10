// BR-08 forensic evidence generator: "fine in the morning, shaking after lunch".
// Two bench runs of the same plan on the stand-in kit, logged with bench.hpp.
// What differs between them is recorded in the answer key, not here.
#include "bench.hpp"
#include "kit_model.hpp"
#include <algorithm>
#include <cmath>
#include <format>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

br08::BenchPlan teamPlan()
{
    br08::BenchPlan p;
    p.signDuty = 0.2;
    p.gains = {0.1207, 0.6188}; // the team's gains from last week's bench session
    return p;
}

void summarise(const std::string& label, const br08::Result& r)
{
    std::vector<double> dt;
    double prev = -1, restV = 0, minV = 1e9, peak = 0, sum = 0, sum2 = 0, loop0 = -1;
    double peakRef = 0, rsum = 0, rsum2 = 0;
    int nRest = 0, sat = 0, nLoop = 0, nh = 0;
    for (const br08::Row& w : r.rows) {
        if (prev >= 0) {
            dt.push_back(1000.0 * (w.tSample - prev));
        }
        prev = w.tSample;
        minV = std::min(minV, w.volts);
        if (w.phase == "rest") {
            restV += w.volts;
            ++nRest;
        }
        if (w.phase == "loop") {
            if (loop0 < 0) {
                loop0 = w.tNominal;
            }
            const double tl = w.tNominal - loop0;
            ++nLoop;
            sat += std::abs(w.duty) >= 0.999 ? 1 : 0;
            if (tl < 1.2) {
                peak = std::max(peak, w.measured);
                peakRef = std::max(peakRef, w.reference);
            }
            if (tl >= 0.6 && tl < 1.2) {
                sum += w.measured;
                sum2 += w.measured * w.measured;
                rsum += w.reference;
                rsum2 += w.reference * w.reference;
                ++nh;
            }
        }
    }
    std::sort(dt.begin(), dt.end());
    const auto late = std::count_if(dt.begin(), dt.end(), [](double x) { return x > 12.0; });
    std::cout << std::format("{}\n", label);
    std::cout << std::format("  sample interval: median {:.2f} ms, max {:.2f} ms, {} of {} "
                             "intervals longer than 12 ms\n",
                             dt[dt.size() / 2], dt.back(), late, dt.size());
    std::cout << std::format("  supply: {:.2f} V at rest, lowest {:.2f} V\n", restV / nRest, minV);
    auto sd = [nh](double a, double a2) {
        return std::sqrt(std::max(0.0, a2 / nh - a * a / nh / nh));
    };
    std::cout << std::format("  loop at 150 rad/s: peak measured {:.1f}, reference {:.1f} rad/s; std "
                             "in the last 0.6 s: measured {:.2f}, reference {:.2f} rad/s\n",
                             peak, peakRef, sd(sum, sum2), sd(rsum, rsum2));
    std::cout << std::format("  duty saturated in {:.1f} % of the loop\n", 100.0 * sat / nLoop);
    std::cout << "  stop: " << r.stop << '\n';
}

} // namespace

int main()
{
    br08::KitParams morning;
    morning.seed = 21;
    br08::KitParams afternoon = morning;
    afternoon.seed = 22;
    afternoon.commandDelay = 0.030;  // firmware 1.4 sends telemetry before the command
    afternoon.burstEvery = 10;       // and its logging task delays every tenth sample
    afternoon.burstLate = 0.004;
    afternoon.batteryOpen = 10.6;    // the battery was not recharged at lunch
    const br08::BenchPlan plan = teamPlan();
    const char* cond[2] = {"bench, 09:40, firmware 1.3, e-stop tested",
                           "bench, 14:10, firmware 1.4, e-stop tested"};
    const char* file[2] = {"forensic_morning.csv", "forensic_afternoon.csv"};
    int i = 0;
    for (const br08::KitParams& p : {morning, afternoon}) {
        br08::KitWorld kit{p};
        const br08::Result r = br08::Bench(kit, plan).run();
        std::ofstream out(file[i]);
        br08::writeLog(out, kit, plan, r, cond[i]);
        summarise(std::string(file[i]) + " (" + cond[i] + ")", r);
        if (i == 1) {
            std::cout << "  excerpt, loop 0.80 to 0.95 s (t_sample, setpoint, measured, duty, "
                         "volts, reference):\n";
            double loop0 = -1;
            for (const br08::Row& w : r.rows) {
                if (w.phase != "loop") {
                    continue;
                }
                if (loop0 < 0) {
                    loop0 = w.tNominal;
                }
                const int ms = static_cast<int>(std::lround(1000 * (w.tNominal - loop0)));
                if (ms >= 800 && ms <= 950) {
                    std::cout << std::format("    {:.5f} {:>6.1f} {:>6.1f} {:>7.4f} {:>5.2f} {:>6.1f}\n",
                                             w.tSample, w.setpoint, w.measured, w.duty, w.volts,
                                             w.reference);
                }
            }
        }
        ++i;
    }
    return 0;
}
