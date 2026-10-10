// BR-08 Listing 7, trap 2: skipping the bench test.
// Three stand-in kits: correctly built; encoder channels swapped; and an encoder with 512
// counts per revolution fitted while the software still says 1024. Each kit is started two
// ways: "floor first" (the closed loop at once, as it ran in the simulator) and with the
// bench sequence (sign test, step test, then a reference check: a short hold at 100 rad/s,
// measured speed against the bench tachometer, before the real setpoint).
#include "bench.hpp"
#include "kit_model.hpp"
#include <cmath>
#include <format>
#include <iostream>
#include <string>

namespace {

constexpr double kLimit = 400.0; // rad/s, the plan's speed limit

struct Outcome
{
    std::string stoppedBy;
    double peakTrue = 0, timeOver = 0;
};

void account(const br08::Result& r, Outcome& o)
{
    for (const br08::Row& w : r.rows) {
        o.peakTrue = std::max(o.peakTrue, std::abs(w.reference));
        o.timeOver += std::abs(w.reference) > kLimit ? 0.01 : 0.0;
    }
}

br08::BenchPlan drivePlan()
{
    br08::BenchPlan p;
    p.benchSteps = false;
    p.signDuty = 0.2;
    p.loopSeconds = 2.0;
    p.profile = {{2.0, 250.0}};
    return p;
}

Outcome floorFirst(br08::KitParams kp)
{
    br08::KitWorld kit{kp};
    const br08::Result r = br08::Bench(kit, drivePlan()).run();
    Outcome o{r.stop, 0, 0};
    account(r, o);
    return o;
}

Outcome benchFirst(br08::KitParams kp)
{
    br08::KitWorld kit{kp};
    Outcome o;
    br08::BenchPlan check;
    check.signDuty = 0.2;
    check.loopSeconds = 1.0;
    check.profile = {{1.0, 100.0}};
    const br08::Result a = br08::Bench(kit, check).run(); // sign, step, hold at 100 rad/s
    account(a, o);
    if (a.stop != "completed") {
        o.stoppedBy = a.stop;
        return o;
    }
    double m = 0, ref = 0;
    int n = 0;
    for (const br08::Row& w : a.rows) {
        if (w.phase == "loop" && w.setpoint == 100.0 && n < 50) { // first 0.5 s at 100 rad/s
            ++n;
        } else if (w.phase == "loop" && w.setpoint == 100.0) {
            m += w.measured;
            ref += w.reference;
        }
    }
    const double ratio = m / ref;
    if (std::abs(ratio - 1.0) > 0.05) {
        o.stoppedBy = std::format("reference check failed (measured/reference {:.3f})", ratio);
        return o;
    }
    const br08::Result b = br08::Bench(kit, drivePlan()).run();
    account(b, o);
    o.stoppedBy = std::format("{} (reference check passed, ratio {:.3f})", b.stop, ratio);
    return o;
}

} // namespace

int main()
{
    br08::KitParams good;
    br08::KitParams swapped;
    swapped.encoderSwapped = true;
    br08::KitParams wrongEncoder;
    wrongEncoder.cprTrue = 512.0;
    std::cout << std::format("speed limit in the plan: {} rad/s; overspeed stop at |measured| > "
                             "{} rad/s\n\n",
                             kLimit, br08::BenchPlan{}.overspeed);
    for (const auto& [name, kp] : {std::pair{"correct kit", good},
                                   std::pair{"encoder swapped", swapped},
                                   std::pair{"512-count encoder", wrongEncoder}}) {
        const Outcome f = floorFirst(kp);
        const Outcome b = benchFirst(kp);
        std::cout << name << '\n';
        std::cout << std::format("  floor first : peak true speed {:>6.1f} rad/s, {:.2f} s above "
                                 "the limit; ended by: {}\n",
                                 f.peakTrue, f.timeOver, f.stoppedBy);
        std::cout << std::format("  bench first : peak true speed {:>6.1f} rad/s, {:.2f} s above "
                                 "the limit; ended by: {}\n",
                                 b.peakTrue, b.timeOver, b.stoppedBy);
    }
    return 0;
}
