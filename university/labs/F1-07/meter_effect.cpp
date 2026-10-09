// How a meter changes what it measures (ideal model, exercise numbers).
// Part 1: a voltmeter with input resistance Rm across the bottom resistor
// of a divider (6 V, two equal resistors R). The meter is in parallel
// with that resistor, so the bottom becomes R || Rm.
// Part 2: an ammeter with internal resistance Rs placed in series with a
// 100 ohm resistor on a 1 V supply.
#include <iomanip>
#include <iostream>

double parallel(double a, double b)
{
    return 1.0 / (1.0 / a + 1.0 / b);
}

int main()
{
    const double supply = 6.0;
    const double dividerResistors[] = {1e3, 1e5, 1e6, 1e7};
    const double meterInputs[] = {1e6, 1e7};

    std::cout << std::fixed << std::setprecision(3);
    std::cout << "Part 1: voltmeter on a divider (true answer always 3.000 V)\n";
    for (const double rm : meterInputs) {
        for (const double r : dividerResistors) {
            const double bottom = parallel(r, rm);
            const double reading = supply * bottom / (r + bottom);
            std::cout << "  Rm = " << std::setw(12) << std::setprecision(0) << rm
                      << "  R = " << std::setw(10) << r << std::setprecision(3)
                      << "  reading = " << reading << " V  error = "
                      << std::setprecision(1) << 100.0 * (reading - 3.0) / 3.0 << " %"
                      << std::setprecision(3) << "\n";
        }
    }

    const double ammeterSupply = 1.0;
    const double load = 100.0;
    const double shunts[] = {0.1, 1.0, 10.0};
    const double trueMilliamps = ammeterSupply / load * 1000.0;
    std::cout << "Part 2: ammeter in series (true answer without meter " << trueMilliamps
              << " mA)\n";
    for (const double rs : shunts) {
        const double reading = ammeterSupply / (load + rs) * 1000.0;
        std::cout << "  Rs = " << std::setw(5) << std::setprecision(1) << rs
                  << " ohms  reading = " << std::setprecision(3) << reading << " mA  error = "
                  << std::setprecision(1) << 100.0 * (reading - trueMilliamps) / trueMilliamps
                  << " %" << std::setprecision(3) << "\n";
    }
    return 0;
}
