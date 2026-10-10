// One analog signal, seen three ways.
// The analog value: a smooth wave between 0 V and 5 V (exercise numbers).
// A 1-bit digital view: HIGH if the value is above one threshold (2.5 V).
// A 3-bit view: the 0..5 V range cut into 8 equal steps, codes 0 to 7.
#include <cmath>
#include <iomanip>
#include <iostream>
#include <numbers>

int main()
{
    const double fullScale = 5.0;
    const double threshold = 2.5;
    const int levels = 8;
    const int samples = 12;

    std::cout << std::fixed << std::setprecision(3);
    std::cout << " sample   analog (V)   1-bit   3-bit code   code in binary\n";
    for (int k = 0; k < samples; ++k) {
        const double phase = 2.0 * std::numbers::pi * (k + 0.5) / samples;
        const double volts = 2.5 - 2.5 * std::cos(phase);
        const int bit = volts > threshold ? 1 : 0;
        int code = static_cast<int>(volts / fullScale * levels);
        if (code > levels - 1) {
            code = levels - 1;
        }
        std::cout << std::setw(7) << k << std::setw(13) << volts << std::setw(8) << bit
                  << std::setw(13) << code << "              " << ((code >> 2) & 1)
                  << ((code >> 1) & 1) << (code & 1) << "\n";
    }
    return 0;
}
