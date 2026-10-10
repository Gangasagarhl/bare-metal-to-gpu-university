// F1-72 Listing 2: a first-order digital low-pass filter (exponential moving
// average), y[n] = y[n-1] + a * (x[n] - y[n-1]), with a = 0.1 at 1000 samples/s.
// For sine waves of several frequencies, measures the output amplitude after the
// filter has settled and compares it with the formula
// |H(f)| = a / |1 - (1 - a) e^(-j 2 pi f / fs)|.
#include <cmath>
#include <complex>
#include <cstdio>
#include <initializer_list>
#include <numbers>

int main()
{
    const double fs = 1000.0;
    const double a = 0.1;
    std::printf("%8s %12s %12s %10s\n", "f Hz", "measured", "formula", "in dB");
    for (double f : {1.0, 5.0, 10.0, 17.0, 50.0, 87.0, 174.0}) {
        double y = 0.0;
        double peak = 0.0;
        const int n = 4000;
        for (int k = 0; k < n; ++k) {
            const double x = std::sin(2.0 * std::numbers::pi * f * k / fs);
            y += a * (x - y);
            if (k >= n / 2) {
                peak = std::fabs(y) > peak ? std::fabs(y) : peak;
            }
        }
        const std::complex<double> z = std::polar(1.0, -2.0 * std::numbers::pi * f / fs);
        const double h = a / std::abs(1.0 - (1.0 - a) * z);
        std::printf("%8.0f %12.4f %12.4f %10.2f\n", f, peak, h, 20.0 * std::log10(h));
    }
    std::printf("\nstep response: samples until the output passes 63.2 %% of a step\n");
    double y = 0.0;
    int k = 0;
    while (y < 0.632) {
        y += a * (1.0 - y);
        ++k;
    }
    std::printf("  %d samples = %.0f ms at %.0f samples/s\n", k, k / fs * 1000.0, fs);
    return 0;
}
