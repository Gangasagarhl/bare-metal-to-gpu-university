// F0-66 lab reference run: noise against lag for the two cures of Listing 2.
// For each encoder resolution and each filter time constant tau (or window W) it prints
// the RMS speed error over t = 1..4 s, and the time shift (lag, in 1 ms samples) that
// minimises the error when the estimate is compared with the true speed s samples earlier.
#include <cmath>
#include <format>
#include <iostream>
#include <numbers>
#include <vector>

struct Result
{
    double rms;      // error with no shift
    int lag;         // best shift in samples
};

Result evaluate(const std::vector<double>& estimate, const std::vector<double>& truth)
{
    Result r{0.0, 0};
    double best = 1e300;
    for (int s = 0; s < 80; ++s) {
        double sum = 0.0;
        int count = 0;
        for (std::size_t k = 1000; k < estimate.size(); ++k) {
            const double d = estimate[k] - truth[k - static_cast<std::size_t>(s)];
            sum += d * d;
            ++count;
        }
        const double rms = std::sqrt(sum / count);
        if (s == 0) {
            r.rms = rms;
        }
        if (rms < best) {
            best = rms;
            r.lag = s;
        }
    }
    return r;
}

int main()
{
    const double pi = std::numbers::pi;
    const double dt = 0.001;
    const int n = 4000;
    for (int countsPerRev : {4096, 1024}) {
        const double step = 2.0 * pi / countsPerRev;
        std::vector<double> measured(n + 1);
        std::vector<double> truth(n + 1);
        for (int k = 0; k <= n; ++k) {
            measured[k] = step * std::round(std::sin(pi * k * dt) / step);
            truth[k] = pi * std::cos(pi * k * dt);
        }
        std::cout << std::format("encoder {} counts/rev: one count per sample = {:.3f} rad/s\n", countsPerRev, step / dt);
        for (double tau : {0.002, 0.005, 0.01, 0.02, 0.05}) {
            const double alpha = dt / (tau + dt);
            double filtered = 0.0;
            std::vector<double> estimate(n + 1, 0.0);
            for (int k = 1; k <= n; ++k) {
                filtered += alpha * ((measured[k] - measured[k - 1]) / dt - filtered);
                estimate[k] = filtered;
            }
            const Result r = evaluate(estimate, truth);
            std::cout << std::format("  low-pass tau = {:>2.0f} ms: RMS error {:.4f} rad/s, lag {:>2} ms\n", tau * 1000.0,
                                     r.rms, r.lag);
        }
        for (int w : {5, 10, 16, 20, 50}) {
            std::vector<double> estimate(n + 1, 0.0);
            for (int k = w; k <= n; ++k) {
                estimate[k] = (measured[k] - measured[k - w]) / (w * dt);
            }
            const Result r = evaluate(estimate, truth);
            std::cout << std::format("  window W = {:>2} samples: RMS error {:.4f} rad/s, lag {:>2} ms\n", w, r.rms, r.lag);
        }
    }
    return 0;
}
