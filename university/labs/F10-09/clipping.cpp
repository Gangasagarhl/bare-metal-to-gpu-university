// F10-09 Listing 2: why clipping is worse than noise. A z accelerometer at rest in a
// hover reads +1 g plus vibration A*sin(2 pi f t); the sensor saturates at +-16 g
// (an exercise range). Averaging removes unclipped vibration but not clipped vibration.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <initializer_list>
#include <numbers>

int main()
{
    constexpr double kRange = 16.0;     // g, full-scale range (exercise value)
    constexpr double kRate = 8000.0;    // samples per second inside the sensor (exercise value)
    constexpr double kF = 190.0;        // Hz, vibration frequency
    std::printf("vib A g  clipped %%   mean g  mean error g   vibration RMS g\n");
    for (double a : {4.0, 8.0, 12.0, 14.9, 16.0, 18.0, 22.0}) {
        const int n = static_cast<int>(kRate);   // one second
        int clipped = 0;
        double sum = 0, sumSq = 0;
        for (int i = 0; i < n; ++i) {
            const double t = i / kRate;
            const double truth = 1.0 + a * std::sin(2 * std::numbers::pi * kF * t);
            const double m = std::clamp(truth, -kRange, kRange);
            clipped += (m != truth) ? 1 : 0;
            sum += m;
            sumSq += m * m;
        }
        const double mean = sum / n;
        const double rms = std::sqrt(std::max(0.0, sumSq / n - mean * mean));
        std::printf("%8.1f %9.2f %8.4f %+13.4f %17.3f\n", a, 100.0 * clipped / n, mean,
                    mean - 1.0, rms);
    }
    return 0;
}
