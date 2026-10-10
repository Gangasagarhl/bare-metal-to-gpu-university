// F9-31 Listing 1: a histogram Bayes filter for a robot in a circular corridor of 30 cells.
// The robot does not know where it starts (uniform belief). Each step it tries to move one
// cell forward (it really moves 0, 1 or 2 cells) and then its door detector reports "door"
// or "wall". The simulated world and the filter use the same models (simulator settings).
#include <array>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <vector>

constexpr int kCells = 30;
using Belief = std::array<double, kCells>;
const std::vector<int> kDoors = {1, 4, 11, 19, 21};

// Motion model p(x_t | x_{t-1}, u = "one forward"): 0, 1 or 2 cells.
constexpr double kMove[3] = {0.1, 0.8, 0.1};
// Measurement model p(z = door | x): 0.9 in front of a door, 0.05 in front of a wall.
constexpr double kDoorIfDoor = 0.9;
constexpr double kDoorIfWall = 0.05;

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

// Prediction: belief_bar(x) = sum over x' of p(x | x', u) * belief(x')   (total probability)
Belief predict(const Belief& bel)
{
    Belief out{};
    for (int from = 0; from < kCells; ++from) {
        for (int step = 0; step < 3; ++step) {
            out[(from + step) % kCells] += kMove[step] * bel[from];
        }
    }
    return out;
}

// Update: belief(x) = eta * p(z | x) * belief_bar(x)   (Bayes' rule)
Belief update(const Belief& belBar, bool sawDoor)
{
    Belief out{};
    double total = 0.0;
    for (int x = 0; x < kCells; ++x) {
        const double pDoor = isDoor(x) ? kDoorIfDoor : kDoorIfWall;
        out[x] = (sawDoor ? pDoor : 1.0 - pDoor) * belBar[x];
        total += out[x];
    }
    for (double& p : out) {
        p /= total;  // eta = 1 / total
    }
    return out;
}

// One character per cell: ' ' below 0.01, '.' below 0.05, ':' below 0.15, '#' otherwise.
std::string picture(const Belief& bel)
{
    std::string s;
    for (double p : bel) {
        s += p < 0.01 ? ' ' : p < 0.05 ? '.' : p < 0.15 ? ':' : '#';
    }
    return s;
}

int main()
{
    std::mt19937 engine(131);
    std::string map(kCells, '-');
    for (int d : kDoors) {
        map[static_cast<std::size_t>(d)] = 'D';
    }
    Belief bel{};
    bel.fill(1.0 / kCells);
    int truth = 8;  // unknown to the filter

    std::cout << "map          |" << map << "|  (D = door)\n";
    std::cout << "step truth z     |belief, one character per cell|  best  p(best)  p(truth)\n";
    for (int t = 1; t <= 24; ++t) {
        // The simulated world: move, then read the door detector.
        const double u = uniform01(engine);
        truth = (truth + (u < kMove[0] ? 0 : u < kMove[0] + kMove[1] ? 1 : 2)) % kCells;
        const double pDoor = isDoor(truth) ? kDoorIfDoor : kDoorIfWall;
        const bool sawDoor = uniform01(engine) < pDoor;

        // The filter: predict with the motion model, update with the reading.
        bel = update(predict(bel), sawDoor);

        int best = 0;
        for (int x = 1; x < kCells; ++x) {
            if (bel[x] > bel[best]) {
                best = x;
            }
        }
        std::cout << std::setw(4) << t << std::setw(6) << truth << "  "
                  << (sawDoor ? "door" : "wall")
                  << "  |" << picture(bel) << "|" << std::setw(5) << best << std::fixed
                  << std::setprecision(3) << std::setw(9) << bel[best] << std::setw(9) << bel[truth]
                  << '\n';
    }
    return 0;
}
