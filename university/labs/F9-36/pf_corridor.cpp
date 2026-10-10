// F9-36 Listing 2: a particle filter for the corridor robot of F9-31, now with a continuous
// position on a 30 m ring. Doors are the 1 m stretches starting at 1, 4, 11, 19 and 21 m.
// The robot tries to move 1 m per step (true move: 1 m + Gaussian noise, sd 0.3 m). The same
// data are also processed by a fine grid Bayes filter (0.1 m cells) for comparison, and the
// particle filter is repeated with 50, 500 and 5000 particles.
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <random>
#include <vector>

constexpr double kLength = 30.0;
const std::vector<double> kDoors = {1.0, 4.0, 11.0, 19.0, 21.0};
constexpr double kMoveSd = 0.3;
constexpr double kDoorIfDoor = 0.9, kDoorIfWall = 0.05;
constexpr int kSteps = 30;

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

double likelihood(bool sawDoor, double x)
{
    const double p = atDoor(x) ? kDoorIfDoor : kDoorIfWall;
    return sawDoor ? p : 1.0 - p;
}

double ringDist(double a, double b)  // shortest distance on the ring
{
    const double d = std::fabs(a - b);
    return std::min(d, kLength - d);
}

struct StepLog
{
    double mass, ess;
    bool resampled;
    long distinct;
};

std::vector<StepLog> particleFilter(int n, const std::vector<double>& truth,
                                    const std::vector<bool>& sawDoor)
{
    std::mt19937 filter(136);  // the filter's own random numbers
    std::vector<double> px(static_cast<std::size_t>(n));
    std::vector<double> w(static_cast<std::size_t>(n), 1.0 / n);
    for (double& x : px) {
        x = kLength * uniform01(filter);  // global uncertainty: anywhere on the ring
    }
    std::vector<StepLog> log;
    for (std::size_t t = 0; t < truth.size(); ++t) {
        double total = 0.0;
        for (std::size_t i = 0; i < px.size(); ++i) {
            px[i] = ring(px[i] + 1.0 + kMoveSd * gaussian(filter));  // sample the motion model
            w[i] *= likelihood(sawDoor[t], px[i]);                    // weight by the reading
            total += w[i];
        }
        double sumSq = 0.0;
        for (double& wi : w) {
            wi /= total;
            sumSq += wi * wi;
        }
        StepLog s{0.0, 1.0 / sumSq, false, 0};
        for (std::size_t i = 0; i < px.size(); ++i) {
            if (ringDist(px[i], truth[t]) <= 1.0) {
                s.mass += w[i];
            }
        }
        if (s.ess < n / 2.0) {  // resample only when the weights have become uneven
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
            w.assign(px.size(), 1.0 / n);
            s.resampled = true;
        }
        std::vector<double> sorted = px;
        std::sort(sorted.begin(), sorted.end());
        s.distinct = std::unique(sorted.begin(), sorted.end()) - sorted.begin();
        log.push_back(s);
    }
    return log;
}

std::vector<double> gridFilter(const std::vector<double>& truth, const std::vector<bool>& sawDoor)
{
    const int cells = 300;  // 0.1 m cells
    std::vector<double> grid(cells, 1.0 / cells);
    std::vector<double> mass;
    for (std::size_t t = 0; t < truth.size(); ++t) {
        std::vector<double> bar(cells, 0.0);
        for (int from = 0; from < cells; ++from) {
            for (int step = 0; step <= 25; ++step) {  // Gaussian kernel, 0 to 2.5 m ahead
                const double d = step * 0.1 - 1.0;
                bar[static_cast<std::size_t>((from + step) % cells)] +=
                    std::exp(-d * d / (2.0 * kMoveSd * kMoveSd)) *
                    grid[static_cast<std::size_t>(from)];
            }
        }
        double total = 0.0;
        for (int c = 0; c < cells; ++c) {
            grid[static_cast<std::size_t>(c)] = bar[static_cast<std::size_t>(c)] *
                                                likelihood(sawDoor[t], (c + 0.5) * 0.1);
            total += grid[static_cast<std::size_t>(c)];
        }
        double m = 0.0;
        for (int c = 0; c < cells; ++c) {
            grid[static_cast<std::size_t>(c)] /= total;
            if (ringDist((c + 0.5) * 0.1, truth[t]) <= 1.0) {
                m += grid[static_cast<std::size_t>(c)];
            }
        }
        mass.push_back(m);
    }
    return mass;
}

int main()
{
    std::mt19937 world(436);       // the simulated world: true motion and door readings
    std::vector<double> truth;
    std::vector<bool> sawDoor;
    double x = 8.5;
    for (int t = 0; t < kSteps; ++t) {
        x = ring(x + 1.0 + kMoveSd * gaussian(world));
        truth.push_back(x);
        sawDoor.push_back(uniform01(world) < (atDoor(x) ? kDoorIfDoor : kDoorIfWall));
    }
    const std::vector<double> grid = gridFilter(truth, sawDoor);
    const std::vector<StepLog> pf = particleFilter(500, truth, sawDoor);
    std::cout << "step  truth  z      mass within 1 m of truth   ESS before  resampled  distinct\n"
              << "                    PF (500)   grid\n";
    for (std::size_t t = 0; t < truth.size(); ++t) {
        std::cout << std::fixed << std::setw(4) << t + 1 << std::setprecision(2) << std::setw(7)
                  << truth[t] << "  " << (sawDoor[t] ? "door" : "wall") << std::setprecision(3)
                  << std::setw(10) << pf[t].mass << std::setw(10) << grid[t]
                  << std::setprecision(1) << std::setw(14) << pf[t].ess << std::setw(10)
                  << (pf[t].resampled ? "yes" : "no") << std::setw(10) << pf[t].distinct << '\n';
    }
    std::cout << "\nparticles  mean |PF mass - grid mass| over all steps\n";
    for (int n : {50, 500, 5000}) {
        const std::vector<StepLog> run = particleFilter(n, truth, sawDoor);
        double diff = 0.0;
        for (std::size_t t = 0; t < truth.size(); ++t) {
            diff += std::fabs(run[t].mass - grid[t]);
        }
        std::cout << std::setw(9) << n << std::setprecision(4) << std::setw(10)
                  << diff / static_cast<double>(truth.size()) << '\n';
    }
    return 0;
}
