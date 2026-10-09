// F0-66 Listing 2: speed from a quantised, noisy encoder at 1 kHz.
// True angle theta(t) = sin(2 pi 0.5 t) rad. The encoder rounds the angle to steps of
// 2 pi / 4096 rad (a pretend encoder). We estimate the speed three ways:
//   raw:      backward difference (theta[k] - theta[k-1]) / dt
//   filtered: the raw difference passed through a first-order low-pass filter
//   slow:     backward difference over 20 samples
// and compare each with the true speed, the exact derivative.
#include <cmath>
#include <format>
#include <iostream>
#include <numbers>
#include <vector>

int main()
{
    const double pi = std::numbers::pi;
    const double dt = 0.001;
    const double step = 2.0 * pi / 4096.0;
    const double tau = 0.01;                      // filter time constant, s
    const double alpha = dt / (tau + dt);         // filter weight per sample
    const int n = 4000;
    std::vector<double> measured(n + 1);
    for (int k = 0; k <= n; ++k) {
        const double t = k * dt;
        measured[k] = step * std::round(std::sin(2.0 * pi * 0.5 * t) / step);
    }
    double filtered = 0.0;
    double sumRaw = 0.0;
    double sumFilt = 0.0;
    double sumSlow = 0.0;
    double maxRaw = 0.0;
    int count = 0;
    std::cout << "  t (s)   true (rad/s)   raw diff   filtered   20-sample diff\n";
    for (int k = 20; k <= n; ++k) {
        const double t = k * dt;
        const double truth = 2.0 * pi * 0.5 * std::cos(2.0 * pi * 0.5 * t);
        const double raw = (measured[k] - measured[k - 1]) / dt;
        filtered += alpha * (raw - filtered);
        const double slow = (measured[k] - measured[k - 20]) / (20.0 * dt);
        if (k >= 1000) {                            // skip the first second (filter start-up)
            sumRaw += (raw - truth) * (raw - truth);
            sumFilt += (filtered - truth) * (filtered - truth);
            sumSlow += (slow - truth) * (slow - truth);
            maxRaw = std::max(maxRaw, std::abs(raw - truth));
            ++count;
        }
        if (k >= 2250 && k <= 2260) {
            std::cout << std::format("{:>7.3f} {:>14.4f} {:>10.4f} {:>10.4f} {:>16.4f}\n", t, truth, raw, filtered,
                                     slow);
        }
    }
    std::cout << std::format("encoder step = {:.6f} rad; one step in one sample looks like {:.4f} rad/s\n", step,
                             step / dt);
    std::cout << std::format("RMS error over t = 1..4 s: raw {:.4f}, filtered {:.4f}, 20-sample {:.4f} rad/s\n",
                             std::sqrt(sumRaw / count), std::sqrt(sumFilt / count), std::sqrt(sumSlow / count));
    std::cout << std::format("largest raw error: {:.4f} rad/s\n", maxRaw);
    return 0;
}
