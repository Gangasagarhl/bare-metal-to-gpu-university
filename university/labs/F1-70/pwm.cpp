// F1-70 Listing 2: driving the F1-69 pretend motor with PWM through an H-bridge.
// During "on" the bridge connects the 6 V pretend supply (forward); during "off"
// both low-side switches are on (brake-style recirculation), so the motor sees 0 V.
// Prints the average voltage, the mean speed and the current ripple for two
// pretend PWM frequencies. Motor parameters as in F1-69 (pretend values).
#include <cstdio>
#include <initializer_list>
#include <numbers>

int main()
{
    const double r = 2.0;
    const double l = 1.0e-3;
    const double ke = 0.01;
    const double kt = 0.01;
    const double j = 1.0e-5;
    const double b = 1.0e-6;
    const double supply = 6.0;
    const double dt = 1.0e-7;
    std::printf("%8s %6s %8s %10s %18s\n", "pwm Hz", "duty", "avg V", "speed rpm",
                "current min..max A");
    for (double pwmHz : {500.0, 20000.0}) {
        for (double duty : {0.25, 0.50, 0.75}) {
            double i = 0.0;
            double w = 0.0;
            double lo = 1e9;
            double hi = -1e9;
            double wSum = 0.0;
            long wCount = 0;
            const long steps = static_cast<long>(1.0 / dt);  // 1 s
            for (long k = 0; k < steps; ++k) {
                const double t = k * dt;
                const double phase = t * pwmHz - static_cast<long>(t * pwmHz);
                const double v = phase < duty ? supply : 0.0;
                const double di = (v - r * i - ke * w) / l;
                const double dw = (kt * i - b * w) / j;
                i += di * dt;
                w += dw * dt;
                if (t > 0.98) {  // last 20 ms: steady state
                    lo = i < lo ? i : lo;
                    hi = i > hi ? i : hi;
                    wSum += w;
                    ++wCount;
                }
            }
            std::printf("%8.0f %5.0f%% %8.2f %10.0f %8.3f..%-8.3f\n", pwmHz, duty * 100.0,
                        supply * duty, wSum / wCount * 60.0 / (2.0 * std::numbers::pi), lo, hi);
        }
    }
    return 0;
}
