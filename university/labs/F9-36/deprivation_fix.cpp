// F9-36 forensic answer-key run: deprivation.cpp with the two fixes of the answer key:
// motion noise sd 0.3 m in the particles again, and 2 % of the particles re-drawn uniformly
// at every step (a simple guard against kidnapping). Same world, same readings.
// Resampling still happens after every reading.
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <random>
#include <vector>

constexpr double kLength = 30.0;
const std::vector<double> kDoors = {1.0, 4.0, 11.0, 19.0, 21.0};
constexpr double kDoorIfDoor = 0.9, kDoorIfWall = 0.05;
constexpr double kFilterMoveSd = 0.3;    // fixed: the motion model's noise is back
constexpr double kRandomShare = 0.02;    // share of particles re-drawn uniformly each step

double uniform01(std::mt19937& engine)
{
    return (static_cast<double>(engine()) + 0.5) / 4294967296.0;
}

double gaussian(std::mt19937& engine)
{
    const double u1 = uniform01(engine);
    const double u2 = uniform01(engine);
    return std::sqrt(-2.0 * std::log(u1)) * std::cos(2.0 * std::acos(-1.0) * u2);
}

double ring(double x)
{
    x = std::fmod(x, kLength);
    return x < 0.0 ? x + kLength : x;
}

bool atDoor(double x)
{
    for (double d : kDoors) {
        if (x >= d && x < d + 1.0) {
            return true;
        }
    }
    return false;
}

double ringDist(double a, double b)
{
    const double d = std::fabs(a - b);
    return std::min(d, kLength - d);
}

int main()
{
    std::mt19937 world(436);
    std::mt19937 filter(236);
    const int n = 500;
    std::vector<double> px(n);
    for (double& x : px) {
        x = kLength * uniform01(filter);
    }
    double truth = 8.5;
    std::cout << "step  truth  z     estimate  distinct  ESS    mass within 1 m of truth\n";
    for (int t = 1; t <= 40; ++t) {
        truth = ring(truth + 1.0 + 0.3 * gaussian(world));
        if (t == 20) {
            truth = ring(truth + 13.0);  // kidnapped
        }
        const bool saw = uniform01(world) < (atDoor(truth) ? kDoorIfDoor : kDoorIfWall);
        std::vector<double> w(n);
        double total = 0.0;
        for (int i = 0; i < n; ++i) {
            if (uniform01(filter) < kRandomShare) {
                px[i] = kLength * uniform01(filter);
            }
            px[i] = ring(px[i] + 1.0 + kFilterMoveSd * gaussian(filter));
            const double p = atDoor(px[i]) ? kDoorIfDoor : kDoorIfWall;
            w[i] = saw ? p : 1.0 - p;
            total += w[i];
        }
        double sumSq = 0.0, mass = 0.0;
        std::vector<double> bins(30, 0.0);
        for (int i = 0; i < n; ++i) {
            w[i] /= total;
            sumSq += w[i] * w[i];
            if (ringDist(px[i], truth) <= 1.0) {
                mass += w[i];
            }
            bins[static_cast<std::size_t>(px[i]) % 30] += w[i];
        }
        const auto best = std::max_element(bins.begin(), bins.end()) - bins.begin();
        // Resample after every reading (systematic).
        std::vector<double> next;
        const double u0 = uniform01(filter) / n;
        double cumulative = w[0];
        std::size_t i = 0;
        for (int m = 0; m < n; ++m) {
            while (u0 + static_cast<double>(m) / n > cumulative && i + 1 < px.size()) {
                cumulative += w[++i];
            }
            next.push_back(px[i]);
        }
        px = next;
        std::vector<double> sorted = px;
        std::sort(sorted.begin(), sorted.end());
        const long distinct = std::unique(sorted.begin(), sorted.end()) - sorted.begin();
        if (t <= 3 || t >= 16) {
            std::cout << std::fixed << std::setw(4) << t << std::setprecision(2) << std::setw(7)
                      << truth << "  " << (saw ? "door" : "wall") << std::setw(6) << best << "-"
                      << std::left << std::setw(4) << best + 1 << std::right << std::setw(9)
                      << distinct << std::setprecision(1) << std::setw(7) << 1.0 / sumSq
                      << std::setprecision(3) << std::setw(10) << mass << '\n';
        }
    }
    return 0;
}
