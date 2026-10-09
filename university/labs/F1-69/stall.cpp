// F1-69 forensic evidence generator: "The motor that hums but will not turn".
// The robot's drive log: for each PWM duty the controller tried, the average motor
// voltage (6 V pretend supply x duty), the speed and current after 1 s, and the
// copper loss i^2 R. Same pretend motor as Listing 1, now driving a gearbox whose
// friction needs a pretend 0.02 N m before anything moves. Synthetic log.
#include <cstdio>
#include <numbers>

int main()
{
    const double r = 2.0;
    const double l = 1.0e-3;
    const double ke = 0.01;
    const double kt = 0.01;
    const double j = 1.0e-5;
    const double b = 1.0e-6;
    const double breakaway = 0.02;  // N m (pretend gearbox friction)
    const double dt = 1.0e-5;
    std::printf("%6s %8s %10s %10s %12s\n", "duty", "avg V", "speed rpm", "current A",
                "i^2 R W");
    for (int duty = 10; duty <= 100; duty += 10) {
        const double v = 6.0 * duty / 100.0;
        double i = 0.0;
        double w = 0.0;
        for (int k = 0; k < 100000; ++k) {
            const double di = (v - r * i - ke * w) / l;
            const double torque = kt * i - b * w;
            double dw = 0.0;
            if (w > 0.0 || torque > breakaway) {
                dw = (torque - breakaway) / j;
            }
            i += di * dt;
            w += dw * dt;
            if (w < 0.0) {
                w = 0.0;
            }
        }
        std::printf("%5d%% %8.2f %10.0f %10.3f %12.3f\n", duty, v,
                    w * 60.0 / (2.0 * std::numbers::pi), i, i * i * r);
    }
    return 0;
}
