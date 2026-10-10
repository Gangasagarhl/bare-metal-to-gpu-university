// occgrid.hpp - a log-odds occupancy grid with the inverse sensor model of F9-52.
#pragma once
#include "house.hpp"
#include <cstdio>
#include <string>
#include <vector>

namespace rb {

struct GridParams
{
    double lOcc = 0.85;     // log-odds added to the cell where a beam ended (p = 0.70)
    double lFree = -0.40;   // log-odds added to each cell a beam passed through (p = 0.40)
    double lMin = -4.0;     // clamp, so a cell can change its mind later
    double lMax = 4.0;
    bool maxRangeIsHit = false;   // the F9-52 forensic bug: treat "no return" as an obstacle
};

class OccGrid
{
public:
    explicit OccGrid(GridParams p = {}) : p_(p), l_(kW * kH, 0.0) {}

    double logOdds(int i, int j) const { return l_[j * kW + i]; }
    double prob(int i, int j) const { return 1.0 - 1.0 / (1.0 + std::exp(logOdds(i, j))); }

    // Add one scan taken at pose p (beams evenly spaced over a full turn).
    void integrate(const Pose& p, const std::vector<double>& z, double maxRange)
    {
        const int n = static_cast<int>(z.size());
        const int i0 = cellOf(p.x);
        const int j0 = cellOf(p.y);
        for (int k = 0; k < n; ++k) {
            const double ang = p.th + 2.0 * kPi * k / n;
            const bool hit = z[k] < maxRange || p_.maxRangeIsHit;
            const int i1 = cellOf(p.x + z[k] * std::cos(ang));
            const int j1 = cellOf(p.y + z[k] * std::sin(ang));
            traverse(i0, j0, i1, j1, hit);
        }
    }

    // 'O' occupied (p > 0.65), '.' free (p < 0.35), ' ' unknown.
    char symbol(int i, int j) const
    {
        const double q = prob(i, j);
        return q > 0.65 ? 'O' : (q < 0.35 ? '.' : ' ');
    }

private:
    static int cellOf(double v) { return static_cast<int>(std::floor(v / kCell)); }

    void add(int i, int j, double d)
    {
        if (i < 0 || j < 0 || i >= kW || j >= kH) { return; }
        double& c = l_[j * kW + i];
        c = std::clamp(c + d, p_.lMin, p_.lMax);
    }

    // Bresenham's line from the robot's cell to the end cell: every cell before
    // the end gets lFree; the end cell gets lOcc if the beam hit something.
    void traverse(int i0, int j0, int i1, int j1, bool hit)
    {
        const int di = std::abs(i1 - i0);
        const int dj = -std::abs(j1 - j0);
        const int si = i0 < i1 ? 1 : -1;
        const int sj = j0 < j1 ? 1 : -1;
        int err = di + dj;
        int i = i0;
        int j = j0;
        while (i != i1 || j != j1) {
            add(i, j, p_.lFree);
            const int e2 = 2 * err;
            if (e2 >= dj) { err += dj; i += si; }
            if (e2 <= di) { err += di; j += sj; }
        }
        add(i1, j1, hit ? p_.lOcc : p_.lFree);
    }

    GridParams p_;
    std::vector<double> l_;
};

// Compare a map with the true house. "Known" = not unknown.
struct Score { int known = 0, falseOcc = 0, falseOccFar = 0, falseFree = 0, occCorrect = 0; };

// True if a cell or one of its 8 neighbours is occupied in the true house.
inline bool nearWall(const Grid& truth, int i, int j)
{
    for (int b = -1; b <= 1; ++b) {
        for (int a = -1; a <= 1; ++a) {
            if (truth.occ(i + a, j + b)) { return true; }
        }
    }
    return false;
}

inline Score compare(const OccGrid& m, const Grid& truth)
{
    Score s;
    for (int j = 0; j < kH; ++j) {
        for (int i = 0; i < kW; ++i) {
            const char c = m.symbol(i, j);
            if (c == ' ') { continue; }
            ++s.known;
            if (c == 'O' && !truth.occ(i, j)) { ++s.falseOcc; }
            if (c == 'O' && !nearWall(truth, i, j)) { ++s.falseOccFar; }
            if (c == '.' && truth.occ(i, j)) { ++s.falseFree; }
            if (c == 'O' && truth.occ(i, j)) { ++s.occCorrect; }
        }
    }
    return s;
}

// Print the map, top row first (y grows upwards), every second column and row
// when `half` is set so that it fits a narrow page.
inline void printMap(const OccGrid& m, bool half)
{
    const int step = half ? 2 : 1;
    for (int j = kH - 1; j >= 0; j -= step) {
        std::string row;
        for (int i = 0; i < kW; i += step) {
            char c = m.symbol(i, j);
            if (half) {   // a 2x2 block shows 'O' if any cell is 'O', '.' if all known cells are free
                char best = ' ';
                for (int b = 0; b < 2; ++b) {
                    for (int a = 0; a < 2; ++a) {
                        const char d = m.symbol(i + a, j - b < 0 ? 0 : j - b);
                        if (d == 'O') { best = 'O'; }
                        else if (d == '.' && best == ' ') { best = '.'; }
                    }
                }
                c = best;
            }
            row += c;
        }
        std::printf("|%s|\n", row.c_str());
    }
}

}  // namespace rb
