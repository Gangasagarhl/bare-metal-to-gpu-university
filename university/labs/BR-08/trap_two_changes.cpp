// BR-08 Listing 9, trap 4: changing two things at once.
// The kit oscillates with the simulator-tuned gains (trap 1). An engineer makes two changes
// in one session: (A) halves both gains, and (B) averages the measured speed over several
// samples "to remove the noise". The combined run is compared with the baseline, and then
// each change is run alone.
#include "bench.hpp"
#include "kit_model.hpp"
#include <cmath>
#include <format>
#include <iostream>

namespace {

struct Stats
{
    double over = 0, holdStd = 0;
};

Stats run(double gainScale, int average)
{
    br08::BenchPlan plan;
    plan.benchSteps = false;
    plan.rampPerSecond = 3000.0;
    plan.gains = {gainScale * 0.195 / (80.8 * 0.008), gainScale * 0.195 / (80.8 * 0.008) / 0.195};
    plan.averageSamples = average;
    br08::KitWorld kit{br08::KitParams{}};
    const br08::Result r = br08::Bench(kit, plan).run();
    Stats s;
    double peak = 0, sum = 0, sum2 = 0;
    int n = 0;
    for (const br08::Row& w : r.rows) {
        if (w.tNominal < 1.2) {
            peak = std::max(peak, w.reference);
        }
        if (w.tNominal >= 0.6 && w.tNominal < 1.2) {
            sum += w.reference;
            sum2 += w.reference * w.reference;
            ++n;
        }
    }
    const double target = 150.0 * 1024.0 / 1000.0; // true speed when the measurement says 150
    s.over = 100.0 * (peak - target) / target;
    s.holdStd = std::sqrt(std::max(0.0, sum2 / n - (sum / n) * (sum / n)));
    return s;
}

void print(const char* label, const Stats& s)
{
    std::cout << std::format("{:<44} {:>9.1f} % {:>10.2f}\n", label, s.over, s.holdStd);
}

} // namespace

int main()
{
    constexpr int kAverage = 4;
    std::cout << std::format("kit (stand-in), 150 rad/s hold; change A: gains x 0.5; change B: "
                             "average of {} samples\n\n",
                             kAverage);
    std::cout << "run                                          overshoot   hold std (rad/s)\n";
    print("baseline: simulator-tuned gains", run(1.0, 1));
    print("A and B together (one session, one log)", run(0.5, kAverage));
    std::cout << "\none change at a time:\n";
    print("A alone: gains x 0.5", run(0.5, 1));
    print("B alone: speed averaged", run(1.0, kAverage));
    return 0;
}
