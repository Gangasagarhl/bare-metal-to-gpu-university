// An LED in series with a resistor, solved two ways.
// Curve model: I = Is * (e^(V / Vn) - 1), with PRETEND parameters
// Is = 1e-18 A and Vn = 0.05 V (exercise values, not from a datasheet).
// The loop needs (Vs - V) / R = I(V); we find V by bisection.
// Simple model: the LED always takes a fixed pretend 2 V.
#include <cmath>
#include <iomanip>
#include <iostream>

double ledAmps(double volts)
{
    const double is = 1e-18;
    const double vn = 0.05;
    return is * (std::exp(volts / vn) - 1.0);
}

double solveLedVolts(double supply, double ohms)
{
    double low = 0.0;
    double high = supply;
    for (int i = 0; i < 100; ++i) {
        const double mid = 0.5 * (low + high);
        const double resistorAmps = (supply - mid) / ohms;
        if (ledAmps(mid) > resistorAmps) {
            high = mid;
        } else {
            low = mid;
        }
    }
    return 0.5 * (low + high);
}

int main()
{
    const double supply = 5.0;
    const double pretendFixedVolts = 2.0;
    const double resistors[] = {1000.0, 470.0, 220.0, 100.0, 1.0};

    std::cout << std::fixed << std::setprecision(3);
    std::cout << "supply " << supply << " V\n";
    std::cout << "      R (ohms)   LED V (curve)   I curve (mA)   I simple (mA)\n";
    for (const double ohms : resistors) {
        const double v = solveLedVolts(supply, ohms);
        const double curveMilliamps = (supply - v) / ohms * 1000.0;
        const double simpleMilliamps = (supply - pretendFixedVolts) / ohms * 1000.0;
        std::cout << std::setprecision(0) << std::setw(14) << ohms << std::setprecision(3)
                  << std::setw(16) << v << std::setw(15) << curveMilliamps
                  << std::setw(16) << simpleMilliamps << "\n";
    }
    std::cout << "reversed LED at -5 V: I = " << std::scientific << ledAmps(-5.0) << " A\n";
    return 0;
}
