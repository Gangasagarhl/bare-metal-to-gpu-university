// Two resistors and one supply, connected in series (S) or in parallel (P).
// Prints the equivalent resistance, every voltage and current, and checks
// the two loop rules: voltages around a loop add up to the supply (series)
// and currents into a junction equal the currents out (parallel).
// All values in network.in are exercise numbers.
#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>

int main()
{
    char how = ' ';
    double supply = 0.0;
    double r1 = 0.0;
    double r2 = 0.0;

    std::cout << std::fixed << std::setprecision(3);
    while (std::cin >> how >> supply >> r1 >> r2) {
        if (how == 'S') {
            const double req = r1 + r2;
            const double amps = supply / req;
            const double v1 = amps * r1;
            const double v2 = amps * r2;
            std::cout << std::setprecision(0) << "series   " << r1 << " + " << r2
                      << std::setprecision(3) << ": Req = " << req << " ohms, I = "
                      << amps * 1000.0 << " mA, V1 = " << v1 << " V, V2 = " << v2 << " V";
            std::cout << (std::fabs(v1 + v2 - supply) < 1e-9 ? "  [V1 + V2 = supply: ok]\n"
                                                              : "  [loop rule FAILED]\n");
        } else if (how == 'P') {
            const double req = 1.0 / (1.0 / r1 + 1.0 / r2);
            const double i1 = supply / r1;
            const double i2 = supply / r2;
            const double total = supply / req;
            std::cout << std::setprecision(0) << "parallel " << r1 << " | " << r2
                      << std::setprecision(3) << ": Req = " << req << " ohms, I1 = "
                      << i1 * 1000.0 << " mA, I2 = " << i2 * 1000.0 << " mA, total = "
                      << total * 1000.0 << " mA";
            std::cout << (std::fabs(i1 + i2 - total) < 1e-12 ? "  [I1 + I2 = total: ok]\n"
                                                             : "  [junction rule FAILED]\n");
        }
    }
    return 0;
}
