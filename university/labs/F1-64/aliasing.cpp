// F1-64 Listing 2: sampling a 9 Hz sine wave at two different sample rates.
// Shows aliasing: sampled too slowly, a fast wave looks like a slow one.
// All signals are synthetic (generated here), not measured.
#include <cmath>
#include <cstdio>
#include <initializer_list>
#include <numbers>

// Count how often the samples change sign, then turn that into a frequency:
// a sine wave crosses zero twice per cycle.
double apparentHz(double signalHz, double sampleHz, double seconds)
{
    const int n = static_cast<int>(sampleHz * seconds);
    int crossings = 0;
    double prev = std::sin(2.0 * std::numbers::pi * signalHz * 0.0 + 0.3);
    for (int k = 1; k < n; ++k) {
        const double t = k / sampleHz;
        const double s = std::sin(2.0 * std::numbers::pi * signalHz * t + 0.3);
        if ((s >= 0.0) != (prev >= 0.0)) {
            ++crossings;
        }
        prev = s;
    }
    return crossings / 2.0 / seconds;
}

// The textbook alias formula: distance from the signal to the nearest
// multiple of the sample rate.
double aliasHz(double signalHz, double sampleHz)
{
    const double k = std::round(signalHz / sampleHz);
    return std::fabs(signalHz - k * sampleHz);
}

int main()
{
    const double f = 9.0;
    std::printf("signal: sine wave, %.0f Hz\n\n", f);
    for (double fs : {100.0, 10.0}) {
        std::printf("sample rate %.0f Hz (Nyquist frequency %.1f Hz): first 10 samples\n", fs,
                    fs / 2);
        for (int k = 0; k < 10; ++k) {
            const double t = k / fs;
            std::printf("  t=%.3f s  %+.3f\n", t, std::sin(2.0 * std::numbers::pi * f * t + 0.3));
        }
        std::printf("  zero-crossing estimate over 10 s: %.2f Hz\n", apparentHz(f, fs, 10.0));
        std::printf("  alias formula |f - round(f/fs)*fs|: %.2f Hz\n\n", aliasHz(f, fs));
    }
    std::printf("alias table for fs = 10 Hz\n");
    for (double sig : {1.0, 4.0, 6.0, 9.0, 11.0, 19.0, 21.0}) {
        std::printf("  %4.0f Hz in -> looks like %4.1f Hz\n", sig, aliasHz(sig, 10.0));
    }
    return 0;
}
