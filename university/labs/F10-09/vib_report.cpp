// F10-09 Listing 3: a vibration report for an accelerometer log in CSV (t_s,ax_g,ay_g,az_g):
// per axis the mean, the vibration level (standard deviation), the extremes and the number
// of samples at the sensor limit, then the two strongest frequencies of z (direct DFT).
#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdio>
#include <numbers>
#include <vector>

constexpr double kLimit = 16.0;   // g; the logged sensor's full scale (exercise value)

int report(const char* name)
{
    std::FILE* f = std::fopen(name, "r");
    if (f == nullptr) {
        std::printf("%s not found: run flight_log first\n", name);
        return 1;
    }
    char header[64];
    if (std::fgets(header, sizeof header, f) == nullptr) {
        std::fclose(f);
        return 1;
    }
    std::vector<double> t, ax[3];
    double r[4];
    while (std::fscanf(f, "%lf,%lf,%lf,%lf", &r[0], &r[1], &r[2], &r[3]) == 4) {
        t.push_back(r[0]);
        for (int k = 0; k < 3; ++k) {
            ax[k].push_back(r[k + 1]);
        }
    }
    std::fclose(f);
    const std::size_t n = t.size();
    const double fs = (n - 1) / (t.back() - t.front());
    std::printf("%s: %zu samples at %.0f Hz\n", name, n, fs);
    std::printf("  axis    mean g   vibe g    min g    max g  at limit\n");
    for (int k = 0; k < 3; ++k) {
        double sum = 0, sq = 0, lo = 1e9, hi = -1e9;
        int clip = 0;
        for (double v : ax[k]) {
            sum += v;
            sq += v * v;
            lo = std::min(lo, v);
            hi = std::max(hi, v);
            clip += (std::abs(v) >= kLimit) ? 1 : 0;
        }
        const double mean = sum / n;
        std::printf("  %c    %+8.3f %8.3f %8.3f %8.3f %9d\n", "xyz"[k], mean,
                    std::sqrt(sq / n - mean * mean), lo, hi, clip);
    }
    // amplitude spectrum of z, bins k * fs / n; keep the two largest peaks
    const auto& z = ax[2];
    double mean = 0;
    for (double v : z) {
        mean += v / n;
    }
    std::vector<double> amp(n / 2, 0.0);
    for (std::size_t k = 1; k < n / 2; ++k) {
        std::complex<double> s{0, 0};
        for (std::size_t i = 0; i < n; ++i) {
            s += (z[i] - mean) * std::polar(1.0, -2 * std::numbers::pi * k * i / n);
        }
        amp[k] = 2 * std::abs(s) / n;
    }
    for (int p = 0; p < 2; ++p) {
        const auto it = std::max_element(amp.begin(), amp.end());
        const auto k = static_cast<std::size_t>(it - amp.begin());
        std::printf("  z peak %d: %6.1f Hz  %7.3f g\n", p + 1, k * fs / n, *it);
        for (std::size_t j = (k > 3 ? k - 3 : 0); j <= k + 3 && j < amp.size(); ++j) {
            amp[j] = 0;                                    // remove the peak and its skirt
        }
    }
    return 0;
}

int main()
{
    return report("flightA.csv") + report("flightB.csv");
}
