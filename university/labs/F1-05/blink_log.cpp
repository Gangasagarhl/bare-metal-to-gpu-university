// Forensic evidence generator for "The blink that is ten times too slow".
// A data logger samples the capacitor voltage of an RC charging circuit
// every 2 s, starting when the supply is switched on.
// The design says R = 10 kilo-ohms and C = 100 microfarads.
// The capacitor REALLY fitted is in blink_log.in (the answer key).
#include <cmath>
#include <iomanip>
#include <iostream>

int main()
{
    const double supply = 5.0;
    const double ohms = 10000.0;
    double fittedMicrofarads = 0.0;
    if (!(std::cin >> fittedMicrofarads)) {
        return 1;
    }
    const double tau = ohms * fittedMicrofarads * 1e-6;

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "log: supply " << supply << " V, sample every 2 s\n";
    for (int t = 0; t <= 30; t += 2) {
        const double vc = supply * (1.0 - std::exp(-t / tau));
        std::cout << "t = " << std::setw(2) << t << " s   Vc = " << vc << " V\n";
    }
    return 0;
}
