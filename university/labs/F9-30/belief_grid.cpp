// F9-30 Listing 1: a belief over a corridor of 20 cells, stored as a histogram.
// Part A: a robot that knows it starts in cell 2 moves 4 times; each move is uncertain,
// so the belief spreads. Part B: a belief with two peaks, where the mean is a bad summary.
#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

using Belief = std::vector<double>;  // Belief[i] = probability that the robot is in cell i

// Mean, variance and the most probable cell of a histogram belief.
void summarise(const Belief& bel)
{
    double total = 0.0;
    double mean = 0.0;
    std::size_t best = 0;
    for (std::size_t i = 0; i < bel.size(); ++i) {
        total += bel[i];
        mean += static_cast<double>(i) * bel[i];
        if (bel[i] > bel[best]) {
            best = i;
        }
    }
    double variance = 0.0;
    for (std::size_t i = 0; i < bel.size(); ++i) {
        const double d = static_cast<double>(i) - mean;
        variance += d * d * bel[i];
    }
    std::cout << std::fixed << std::setprecision(3) << "  sum " << total << "  mean " << mean
              << "  sd " << std::sqrt(variance) << "  most probable cell " << best << " (p "
              << bel[best] << ")\n";
}

// Text bar chart: one '#' per 0.02 of probability, cells with p < 0.0005 left blank.
void show(const std::string& label, const Belief& bel)
{
    std::cout << label << '\n';
    for (std::size_t i = 0; i < bel.size(); ++i) {
        if (bel[i] < 0.0005) {
            continue;
        }
        const int bars = static_cast<int>(std::lround(bel[i] / 0.02));
        std::cout << "  cell " << std::setw(2) << i << "  " << std::fixed << std::setprecision(3)
                  << bel[i] << "  " << std::string(static_cast<std::size_t>(bars), '#') << '\n';
    }
    summarise(bel);
}

// One uncertain move "one cell forward": it really moves 0, 1 or 2 cells.
Belief moveForward(const Belief& bel)
{
    const double pStay = 0.1;
    const double pOne = 0.8;
    const double pTwo = 0.1;
    Belief next(bel.size(), 0.0);
    for (std::size_t i = 0; i < bel.size(); ++i) {
        next[i] += pStay * bel[i];
        if (i + 1 < bel.size()) {
            next[i + 1] += pOne * bel[i];
        }
        if (i + 2 < bel.size()) {
            next[i + 2] += pTwo * bel[i];
        }
    }
    return next;
}

int main()
{
    std::cout << "Part A: start known exactly, then four uncertain moves\n";
    Belief bel(20, 0.0);
    bel[2] = 1.0;
    show("start", bel);
    for (int k = 1; k <= 4; ++k) {
        bel = moveForward(bel);
        show("after move " + std::to_string(k), bel);
    }

    std::cout << "\nPart B: two equally likely places (in front of door 1 or door 2)\n";
    Belief twoPeaks(20, 0.0);
    twoPeaks[3] = 0.1;
    twoPeaks[4] = 0.3;
    twoPeaks[5] = 0.1;
    twoPeaks[13] = 0.1;
    twoPeaks[14] = 0.3;
    twoPeaks[15] = 0.1;
    show("two peaks", twoPeaks);
    std::cout << "  probability of the cell nearest the mean (cell 9): " << twoPeaks[9] << '\n';
    return 0;
}
