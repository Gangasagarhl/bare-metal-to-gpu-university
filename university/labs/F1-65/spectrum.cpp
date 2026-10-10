// F1-65 Listing 3: the amplitude spectrum of a logged signal, with a real
// discrete Fourier transform (DFT) written out in full (no library).
// Reads jumpy_imu.csv (written by jumpy_log.cpp) and analyses both columns.
#include <cmath>
#include <complex>
#include <cstdio>
#include <fstream>
#include <numbers>
#include <sstream>
#include <string>
#include <vector>

// One-sided amplitude spectrum: A[k] = 2|X[k]|/N for 0 < k < N/2, with
// X[k] = sum over n of x[n] * exp(-i 2 pi k n / N). The mean is removed first.
std::vector<double> amplitudeSpectrum(std::vector<double> x)
{
    const std::size_t n = x.size();
    double mean = 0.0;
    for (double v : x) {
        mean += v;
    }
    mean /= static_cast<double>(n);
    for (double& v : x) {
        v -= mean;
    }
    std::vector<double> amp(n / 2, 0.0);
    for (std::size_t k = 1; k < n / 2; ++k) {
        std::complex<double> sum = 0.0;
        for (std::size_t i = 0; i < n; ++i) {
            const double angle = -2.0 * std::numbers::pi * static_cast<double>(k * i) / n;
            sum += x[i] * std::polar(1.0, angle);
        }
        amp[k] = 2.0 * std::abs(sum) / static_cast<double>(n);
    }
    return amp;
}

void report(const char* name, const std::vector<double>& amp, double binHz)
{
    std::printf("== %s\n", name);
    std::vector<double> a = amp;
    std::printf("largest peaks:\n");
    for (int p = 0; p < 4; ++p) {
        std::size_t best = 1;
        for (std::size_t k = 1; k < a.size(); ++k) {
            if (a[k] > a[best]) {
                best = k;
            }
        }
        std::printf("  %6.1f Hz  amplitude %.4f g\n", best * binHz, a[best]);
        a[best] = 0.0;
    }
    std::printf("band maxima (each # = 0.01 g, bands of 25 Hz):\n");
    for (std::size_t lo = 0; lo < amp.size(); lo += 25) {
        double m = 0.0;
        for (std::size_t k = lo; k < lo + 25 && k < amp.size(); ++k) {
            m = amp[k] > m ? amp[k] : m;
        }
        const int bars = static_cast<int>(std::lround(m / 0.01));
        std::printf("  %3zu-%3zu Hz %.4f %s\n", lo, lo + 24, m, std::string(bars, '#').c_str());
    }
}

int main()
{
    std::ifstream in("jumpy_imu.csv");
    if (!in) {
        std::printf("jumpy_imu.csv not found: run jumpy_log first\n");
        return 1;
    }
    std::string line;
    std::getline(in, line);  // header
    std::vector<double> t;
    std::vector<double> off;
    std::vector<double> on;
    while (std::getline(in, line)) {
        std::stringstream ss(line);
        std::string a;
        std::string b;
        std::string c;
        std::getline(ss, a, ',');
        std::getline(ss, b, ',');
        std::getline(ss, c, ',');
        t.push_back(std::stod(a));
        off.push_back(std::stod(b));
        on.push_back(std::stod(c));
    }
    const double fs = 1.0 / (t[1] - t[0]);
    const double binHz = fs / static_cast<double>(t.size());
    std::printf("%zu samples, fs = %.1f Hz, bin width %.2f Hz, highest frequency %.1f Hz\n\n",
                t.size(), fs, binHz, fs / 2);
    report("motors off", amplitudeSpectrum(off), binHz);
    report("motors on", amplitudeSpectrum(on), binHz);
    return 0;
}
