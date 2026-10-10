// F9-31 forensic evidence generator: "The robot that is sure it is somewhere else".
// Same corridor and robot as Listing 1, but the filter's door-detector model says the
// detector NEVER reports "door" in front of a wall (0.0), while the simulated detector does
// so with probability 0.05. The log is what the robot's developer saw.
#include <array>
#include <iomanip>
#include <iostream>
#include <random>
#include <vector>

constexpr int kCells = 30;
using Belief = std::array<double, kCells>;
const std::vector<int> kDoors = {1, 4, 11, 19, 21};
constexpr double kMove[3] = {0.1, 0.8, 0.1};
constexpr double kWorldDoorIfWall = 0.05;   // the real detector
constexpr double kFilterDoorIfWall = 0.0;   // what the filter was told
constexpr double kDoorIfDoor = 0.9;

bool isDoor(int cell)
{
    for (int d : kDoors) {
        if (d == cell) {
            return true;
        }
    }
    return false;
}

double uniform01(std::mt19937& engine)
{
    return (static_cast<double>(engine()) + 0.5) / 4294967296.0;
}

int main()
{
    std::mt19937 engine(231);
    Belief bel{};
    bel.fill(1.0 / kCells);
    int truth = 8;
    std::cout << "step truth reading  best p(best)  p(truth)  cells with p > 0\n";
    for (int t = 1; t <= 30; ++t) {
        const double u = uniform01(engine);
        truth = (truth + (u < kMove[0] ? 0 : u < kMove[0] + kMove[1] ? 1 : 2)) % kCells;
        const double pWorld = isDoor(truth) ? kDoorIfDoor : kWorldDoorIfWall;
        const bool sawDoor = uniform01(engine) < pWorld;

        Belief bar{};
        for (int from = 0; from < kCells; ++from) {
            for (int step = 0; step < 3; ++step) {
                bar[(from + step) % kCells] += kMove[step] * bel[from];
            }
        }
        double total = 0.0;
        for (int x = 0; x < kCells; ++x) {
            const double pDoor = isDoor(x) ? kDoorIfDoor : kFilterDoorIfWall;
            bel[x] = (sawDoor ? pDoor : 1.0 - pDoor) * bar[x];
            total += bel[x];
        }
        for (double& p : bel) {
            p /= total;
        }
        int best = 0;
        int nonZero = 0;
        for (int x = 0; x < kCells; ++x) {
            if (bel[x] > bel[best]) {
                best = x;
            }
            if (bel[x] > 0.0) {
                ++nonZero;
            }
        }
        std::cout << std::setw(4) << t << std::setw(6) << truth << "  "
                  << (sawDoor ? "door" : "wall")
                  << std::setw(7) << best << std::fixed << std::setprecision(3) << std::setw(8)
                  << bel[best] << std::setw(10) << bel[truth] << std::setw(8) << nonZero << '\n';
    }
    return 0;
}
