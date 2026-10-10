// F1-64 Listing 1: a model of an ideal n-bit analog-to-digital converter (ADC).
// The university's own model, not any real part. The reference voltage 3.0 V
// is a pretend exercise value; a real ADC's reference, resolution and transfer
// function come from its datasheet.
#include <cmath>
#include <cstdio>
#include <initializer_list>
#include <vector>

// Ideal ADC: code = floor(v / vref * 2^bits), clamped to 0 .. 2^bits - 1.
int convert(double v, double vref, int bits)
{
    const int levels = 1 << bits;
    int code = static_cast<int>(std::floor(v / vref * levels));
    if (code < 0) {
        code = 0;
    }
    if (code > levels - 1) {
        code = levels - 1;
    }
    return code;
}

// The voltage a program should report for a code: the middle of its step.
double codeToVolts(int code, double vref, int bits)
{
    const double lsb = vref / (1 << bits);
    return (code + 0.5) * lsb;
}

int main()
{
    const double vref = 3.0;  // pretend exercise value
    std::printf("Part A: a 3-bit ADC, vref = %.1f V\n", vref);
    std::printf("LSB = vref / 2^3 = %.4f V\n", vref / 8);
    std::printf("%8s %6s %8s %10s %10s\n", "input V", "code", "binary", "reported V", "error V");
    const std::vector<double> inputs = {0.0, 0.20, 0.374, 0.376, 1.00, 1.50, 2.62, 2.99, 3.20};
    for (double v : inputs) {
        const int c = convert(v, vref, 3);
        const double back = codeToVolts(c, vref, 3);
        std::printf("%8.3f %6d      %d%d%d %10.4f %+10.4f\n", v, c, (c >> 2) & 1, (c >> 1) & 1,
                    c & 1, back, back - v);
    }

    std::printf("\nPart B: step size (LSB) for more bits, same vref\n");
    for (int bits : {8, 10, 12, 16}) {
        const double lsb = vref / (1 << bits);
        std::printf("%2d bits: %6d codes, LSB = %.6f V = %8.3f mV\n", bits, 1 << bits, lsb,
                    lsb * 1000.0);
    }

    std::printf("\nPart C: worst quantisation error over a fine sweep (10-bit)\n");
    double worst = 0.0;
    for (int i = 0; i <= 300000; ++i) {
        const double v = vref * i / 300000.0 * 0.999999;
        const double err = std::fabs(codeToVolts(convert(v, vref, 10), vref, 10) - v);
        if (err > worst) {
            worst = err;
        }
    }
    std::printf("worst |error| = %.6f V, half an LSB = %.6f V\n", worst, vref / 1024 / 2);
    return 0;
}
