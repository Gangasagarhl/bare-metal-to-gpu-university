// HW101 course project, reference solution (Lab Engineer, guide 11.5): an RC timer that blinks an
// LED without a microcontroller, predicted as a RANGE and then simulated. Same model as F1-08's
// relaxation.cpp (Schmitt-trigger inverter, R from output to input, C from input to ground).
// Every value is an exercise value: thresholds 3 V / 2 V on 5 V, R = 47 kilo-ohms +-5 %,
// C = 100 microfarads +-20 % (the tolerances are exercise values too; the kit's datasheets rule).
#include <cmath>
#include <iomanip>
#include <iostream>

static double halfPeriodHigh(double tau, double vdd, double upper, double lower) { return tau * std::log((vdd - lower) / (vdd - upper)); }
static double halfPeriodLow(double tau, double upper, double lower) { return tau * std::log(upper / lower); }

int main()
{
    const double vdd = 5.0, upper = 3.0, lower = 2.0;
    const double ohms = 47000.0, farads = 100e-6;
    const double rTol = 0.05, cTol = 0.20;
    const double tau = ohms * farads;

    std::cout << std::fixed << std::setprecision(4);
    std::cout << "Prediction sheet (exercise values): Vdd " << vdd << " V, VT+ " << upper << " V, VT- " << lower
              << " V, R " << std::setprecision(0) << ohms << " ohms, C " << std::setprecision(1) << farads * 1e6
              << " uF, tau " << std::setprecision(4) << tau << " s\n";
    const double tH = halfPeriodHigh(tau, vdd, upper, lower), tL = halfPeriodLow(tau, upper, lower);
    std::cout << "  nominal: HIGH " << tH << " s, LOW " << tL << " s, period " << tH + tL << " s, first HIGH "
              << tau * std::log(vdd / (vdd - upper)) << " s\n";
    const double tauMin = tau * (1.0 - rTol) * (1.0 - cTol), tauMax = tau * (1.0 + rTol) * (1.0 + cTol);
    std::cout << "  range from tolerances: tau " << tauMin << " to " << tauMax << " s, period "
              << halfPeriodHigh(tauMin, vdd, upper, lower) + halfPeriodLow(tauMin, upper, lower) << " to "
              << halfPeriodHigh(tauMax, vdd, upper, lower) + halfPeriodLow(tauMax, upper, lower) << " s\n";
    const double ledForward = 2.0, ledTargetAmps = 0.010;
    const double minOhms = (vdd - ledForward) / ledTargetAmps, chosenOhms = 330.0;
    std::cout << "  LED drive (F1-06, pretend VF " << std::setprecision(1) << ledForward << " V, 10 mA): minimum "
              << minOhms << " ohms, chosen " << chosenOhms << " ohms, current " << std::setprecision(2)
              << (vdd - ledForward) / chosenOhms * 1000.0 << " mA\n" << std::setprecision(4);

    // simulation, 0.1 ms steps, 20 s
    const double dt = 0.0001;
    double vc = 0.0, lastSwitch = 0.0;
    bool high = true;
    int switches = 0;
    double sumHalf = 0.0;
    for (long k = 1; k <= 200000; ++k) {
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
        if (switches > 1) {
            sumHalf += t - lastSwitch;
        }
        if (switches <= 5) {
            std::cout << "  t = " << t << " s  output -> " << (high ? "HIGH (LED on) " : "LOW  (LED off)")
                      << "  half-period " << t - lastSwitch << " s\n";
        }
        lastSwitch = t;
    }
    std::cout << "Simulated: " << switches << " switches in 20 s; mean half-period after the first "
              << sumHalf / (switches - 1) << " s; period " << 2.0 * sumHalf / (switches - 1) << " s\n";
    return 0;
}
