// Check-yourself question 8 of F1-05: Listing 1 with a COARSE time step.
// Two ways: (1) step by step: in each small time step dt the current
// I = (Vs - Vc) / R adds charge I * dt, so Vc rises by I * dt / C;
// (2) the formula Vc(t) = Vs * (1 - e^(-t / (R * C))).
// R, C and Vs are exercise numbers: 10 kilo-ohms, 100 microfarads, 5 V.
#include <cmath>
#include <iomanip>
#include <iostream>

int main()
{
    const double supply = 5.0;
    const double ohms = 10000.0;
    const double farads = 100e-6;
    const double tau = ohms * farads;
    const double dt = 0.1; // deliberately coarse: a tenth of tau

    std::cout << std::fixed << std::setprecision(3);
    std::cout << "tau = R * C = " << tau << " s\n";
    std::cout << "   t (s)   step-by-step (V)   formula (V)   percent of supply\n";
    double vc = 0.0;
    const int stepsPerPrint = 5;
    for (int k = 0; k <= 10 * stepsPerPrint; ++k) {
        if (k % stepsPerPrint == 0) {
            const double t = k * dt;
            const double formula = supply * (1.0 - std::exp(-t / tau));
            std::cout << std::setw(8) << t << std::setw(19) << vc << std::setw(14) << formula
                      << std::setw(17) << std::setprecision(1) << 100.0 * formula / supply
                      << std::setprecision(3) << "\n";
        }
        const double amps = (supply - vc) / ohms;
        vc += amps * dt / farads;
    }
    return 0;
}
