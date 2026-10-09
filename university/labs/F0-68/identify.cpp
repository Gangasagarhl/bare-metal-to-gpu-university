// F0-68 answer-key program: reads the oscillating-motor log (identify.in is an exact copy
// of servo_log.out) and estimates the damping ratio and natural frequency from
// (1) the first overshoot and its time, and (2) the ratio of the first two overshoots.
#include <cmath>
#include <format>
#include <iostream>
#include <numbers>
#include <string>
#include <vector>

int main()
{
    std::vector<double> t;
    std::vector<double> y;
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        const std::size_t space = line.find(' ');
        t.push_back(std::stod(line.substr(0, space)));
        y.push_back(std::stod(line.substr(space + 1)));
    }
    const double target = 1.0;
    // local maxima above the target
    std::vector<std::size_t> peaks;
    for (std::size_t k = 1; k + 1 < y.size(); ++k) {
        if (y[k] > target && y[k] >= y[k - 1] && y[k] > y[k + 1]) {
            peaks.push_back(k);
        }
    }
    if (peaks.size() < 2) {
        std::cout << "fewer than two overshoots found\n";
        return 1;
    }
    const double pi = std::numbers::pi;
    const double mp = (y[peaks[0]] - target) / target;
    const double tp = t[peaks[0]];
    const double lnMp = std::log(mp);
    const double zeta1 = -lnMp / std::sqrt(pi * pi + lnMp * lnMp);
    const double wn1 = pi / (tp * std::sqrt(1.0 - zeta1 * zeta1));
    std::cout << std::format("samples: {}, first peak {:.4f} rad at {:.3f} s, second peak {:.4f} rad at {:.3f} s\n",
                             y.size(), y[peaks[0]], tp, y[peaks[1]], t[peaks[1]]);
    std::cout << std::format("method 1 (overshoot, peak time): Mp = {:.3f}, zeta = {:.3f}, wn = {:.2f} rad/s ({:.2f} Hz)\n",
                             mp, zeta1, wn1, wn1 / (2.0 * pi));
    const double delta = std::log((y[peaks[0]] - target) / (y[peaks[1]] - target));
    const double zeta2 = delta / std::sqrt(4.0 * pi * pi + delta * delta);
    const double period = t[peaks[1]] - t[peaks[0]];
    const double wn2 = (2.0 * pi / period) / std::sqrt(1.0 - zeta2 * zeta2);
    std::cout << std::format("method 2 (two peaks): decrement = {:.3f}, zeta = {:.3f}, period = {:.3f} s, wn = {:.2f} rad/s\n",
                             delta, zeta2, period, wn2);
    return 0;
}
