// An RC blinker with no microcontroller: a model of a Schmitt-trigger
// relaxation oscillator. The output is HIGH (5 V) or LOW (0 V). The
// capacitor charges toward the output through R. When Vc rises above the
// upper threshold the output goes LOW; when Vc falls below the lower
// threshold the output goes HIGH again. Thresholds, R and C are exercise
// numbers, not values from a real part's datasheet.
#include <cmath>
#include <iomanip>
#include <iostream>

int main()
{
    const double vdd = 5.0;
    const double upper = 3.0;
    const double lower = 2.0;
    const double ohms = 10000.0;
    const double farads = 100e-6;
    const double tau = ohms * farads;
    const double dt = 0.0001;

    const double predictedHigh = tau * std::log((vdd - lower) / (vdd - upper));
    const double predictedLow = tau * std::log(upper / lower);
    std::cout << std::fixed << std::setprecision(4);
    std::cout << "predicted: HIGH for " << predictedHigh << " s, LOW for " << predictedLow
              << " s, period " << predictedHigh + predictedLow << " s\n";

    double vc = 0.0;
    bool high = true;
    double lastSwitch = 0.0;
    int switches = 0;
    for (long k = 1; k <= 60000; ++k) {
        const double out = high ? vdd : 0.0;
        vc += (out - vc) / ohms * dt / farads;
        const double t = k * dt;
        if (high && vc > upper) {
            high = false;
        } else if (!high && vc < lower) {
            high = true;
        } else {
            continue;
        }
        ++switches;
        std::cout << "t = " << t << " s  output -> " << (high ? "HIGH (LED on) " : "LOW  (LED off)")
                  << "  time since last switch " << t - lastSwitch << " s\n";
        lastSwitch = t;
    }
    std::cout << "switches in 6 s: " << switches << "\n";
    return 0;
}
