// F9-31 checks: the worked example (a ring of 5 cells, doors at cells 0 and 3) computed
// step by step, so every number in the chapter's text comes from a run.
#include <array>
#include <iomanip>
#include <iostream>
#include <string>

using B = std::array<double, 5>;

void print(const std::string& label, const B& b)
{
    std::cout << std::setw(26) << std::left << label << std::right;
    for (double p : b) {
        std::cout << std::setw(9) << p;
    }
    std::cout << '\n';
}

int main()
{
    std::cout << std::fixed << std::setprecision(4);
    const std::array<bool, 5> door = {true, false, false, true, false};
    const double pDoorIfDoor = 0.9;
    const double pDoorIfWall = 0.05;
    B bel;
    bel.fill(0.2);
    print("prior", bel);

    // Update with z = door.
    B un{};
    double total = 0.0;
    for (int x = 0; x < 5; ++x) {
        un[x] = (door[x] ? pDoorIfDoor : pDoorIfWall) * bel[x];
        total += un[x];
    }
    print("p(z=door|x) * prior", un);
    std::cout << "normaliser 1/eta = " << total << '\n';
    for (int x = 0; x < 5; ++x) {
        bel[x] = un[x] / total;
    }
    print("posterior after 'door'", bel);

    // Predict with "one forward": 0, 1, 2 cells with 0.1, 0.8, 0.1 (ring).
    B bar{};
    for (int from = 0; from < 5; ++from) {
        bar[from] += 0.1 * bel[from];
        bar[(from + 1) % 5] += 0.8 * bel[from];
        bar[(from + 2) % 5] += 0.1 * bel[from];
    }
    print("prediction (belief bar)", bar);

    // Update with z = wall.
    total = 0.0;
    for (int x = 0; x < 5; ++x) {
        un[x] = (door[x] ? 1.0 - pDoorIfDoor : 1.0 - pDoorIfWall) * bar[x];
        total += un[x];
    }
    print("p(z=wall|x) * bar", un);
    std::cout << "normaliser 1/eta = " << total << '\n';
    for (int x = 0; x < 5; ++x) {
        bel[x] = un[x] / total;
    }
    print("posterior after 'wall'", bel);

    // Cost of one filter step for a grid of N cells and a motion kernel of width w.
    std::cout << "operations per step, 1-D grid of 30 cells, kernel 3: " << 30 * 3 + 30 << '\n';
    const double cells2d = (20.0 / 0.05) * (20.0 / 0.05) * 72.0;
    std::cout << "cells for a 20 m x 20 m area, 5 cm grid, 5 deg heading bins: " << cells2d << '\n';
    return 0;
}
