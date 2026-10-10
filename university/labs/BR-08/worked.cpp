// BR-08 Listing 10: the arithmetic of the worked example, so that no number is typed by hand.
// Inputs are the values printed in compare.out and forensic.out (quoted in the chapter).
#include <cmath>
#include <format>
#include <iostream>
#include <tuple>

int main()
{
    constexpr double kPi = 3.141592653589793;
    // A. Calibration: encoder with 1000 counts, software parameter 1024
    const double ratio = 1000.0 / 1024.0;
    std::cout << std::format("A. measured/true = 1000/1024 = {:.4f}; measured 150 -> true {:.1f}, "
                             "measured 250 -> true {:.1f} rad/s; error {:.1f} %\n",
                             ratio, 150.0 / ratio, 250.0 / ratio, 100.0 * (1.0 / ratio - 1.0));

    // B. Timing: speed computed with the nominal 10 ms while the real interval differs
    for (double interval : {10.4, 14.4, 5.6}) {
        const double factor = interval / 10.0;
        std::cout << std::format("B. interval {:.1f} ms: counts are {:.2f} x a 10 ms count; "
                                 "at a true 150 rad/s the loop computes {:.0f} rad/s\n",
                                 interval, factor, 150.0 * factor);
    }

    // C. Static identification from the two holds (reference speed against duty x supply)
    const double w1 = 153.83, v1 = 2.388, w2 = 256.13, v2 = 3.440;
    const double kv = (w2 - w1) / (v2 - v1);
    const double dead = v1 - w1 / kv;
    std::cout << std::format("C. gain = ({:.2f} - {:.2f}) / ({:.3f} - {:.3f}) = {:.1f} rad/s "
                             "per V; deadband = {:.3f} - {:.2f}/{:.1f} = {:.2f} V\n",
                             w2, w1, v2, v1, kv, v1, w1, kv, dead);

    // D. Crossover and delay margin of the speed loop, integrator approximation:
    //    wc = kp * K * (supply / 12) * (measured/true) / tau ; margin = phase left / wc
    struct World
    {
        const char* name;
        double k, tau, supply, scale, delay;
    };
    const World sim{"simulator", 98.04, 0.1961, 12.0, 1.0, 0.010};
    const World kit{"kit      ", kv, 0.322, 11.11, 0.9769, 0.010 + 0.012 + 0.00025};
    for (const auto& [gname, kp, ki] : {std::tuple{"bring-up ", 0.0482, 0.2474},
                                        std::tuple{"sim-tuned", 0.3017, 1.5470}}) {
        for (const World& w : {sim, kit}) {
            const double wc = kp * w.k * (w.supply / 12.0) * w.scale / w.tau;
            const double piLag = std::atan((ki / kp) / wc); // the PI zero's phase lag at wc
            const double margin = (kPi / 2.0 - piLag) / wc;
            std::cout << std::format("D. {} {}: wc = {:5.1f} rad/s, delay margin {:5.1f} ms, "
                                     "loop delay {:4.1f} ms, wc x delay = {:.2f} rad\n",
                                     gname, w.name, wc, 1000.0 * margin, 1000.0 * w.delay,
                                     wc * w.delay);
        }
    }
    return 0;
}
