// F1-72 Listing 1: averaging N noisy readings. The true value is 1.000 V and every
// reading has independent Gaussian noise with standard deviation 10 mV (synthetic).
// For each N, 2000 averages are formed and their spread is measured, then compared
// with the prediction sigma / sqrt(N) from probability (MA202).
#include "noise.h"

#include <cmath>
#include <cstdio>
#include <initializer_list>

int main()
{
    Noise noise(99u);
    const double sigma = 0.010;
    std::printf("%6s %16s %16s\n", "N", "measured sd mV", "sigma/sqrt(N) mV");
    for (int n : {1, 4, 16, 64, 256}) {
        const int trials = 2000;
        double sum = 0.0;
        double sumSq = 0.0;
        for (int t = 0; t < trials; ++t) {
            double avg = 0.0;
            for (int k = 0; k < n; ++k) {
                avg += 1.0 + noise.gaussian(sigma);
            }
            avg /= n;
            sum += avg;
            sumSq += avg * avg;
        }
        const double mean = sum / trials;
        const double sd = std::sqrt((sumSq - trials * mean * mean) / (trials - 1));
        std::printf("%6d %16.3f %16.3f\n", n, sd * 1000.0, sigma / std::sqrt(n) * 1000.0);
    }
    std::printf("\nan offset does not average away: 1.000 V + 25 mV offset, N = 256\n");
    double avg = 0.0;
    for (int k = 0; k < 256; ++k) {
        avg += 1.025 + noise.gaussian(sigma);
    }
    std::printf("  average = %.4f V (still about 25 mV high)\n", avg / 256);
    return 0;
}
