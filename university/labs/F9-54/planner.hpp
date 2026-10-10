// planner.hpp - F9-54: grid graph search (Dijkstra, A*, weighted A*) on a cost grid.
// Used again by F9-55 (reference path) and F9-56 (the global planner of the mini stack).
#pragma once
#include "../F9-52/house.hpp"
#include <cmath>
#include <cstdio>
#include <functional>
#include <limits>
#include <queue>
#include <string>
#include <vector>

namespace rb {

constexpr double kLethal = std::numeric_limits<double>::infinity();

// cost[j * kW + i]: 0 = free, a positive number = extra cost per metre, kLethal = blocked.
struct CostGrid
{
    std::vector<double> cost = std::vector<double>(kW * kH, 0.0);
    double at(int i, int j) const
    {
        return (i < 0 || j < 0 || i >= kW || j >= kH) ? kLethal : cost[j * kW + i];
    }
};

// Block every cell whose centre is within `radius` of an occupied cell's centre:
// a disc-shaped robot of that radius whose centre is in a free cell touches nothing.
inline CostGrid inflate(const Grid& g, double radius)
{
    CostGrid c;
    const int r = static_cast<int>(std::ceil(radius / kCell));
    for (int j = 0; j < kH; ++j) {
        for (int i = 0; i < kW; ++i) {
            if (!g.occ(i, j)) { continue; }
            for (int b = -r; b <= r; ++b) {
                for (int a = -r; a <= r; ++a) {
                    if (std::hypot(a, b) * kCell <= radius && g.inside(i + a, j + b)) {
                        c.cost[(j + b) * kW + (i + a)] = kLethal;
                    }
                }
            }
        }
    }
    return c;
}

struct Cell { int i, j; };

struct PlanResult
{
    std::vector<Cell> path;    // start ... goal; empty if no path
    double cost = 0.0;         // sum of step lengths x (1 + cell cost), in metres
    int expanded = 0;          // nodes taken from the open list and expanded
};

// Octile distance in metres: the exact shortest 8-connected distance on an empty grid.
inline double octile(Cell a, Cell b)
{
    const double dx = std::abs(a.i - b.i);
    const double dy = std::abs(a.j - b.j);
    return kCell * ((dx + dy) + (std::sqrt(2.0) - 2.0) * std::fmin(dx, dy));
}

// hWeight = 0: Dijkstra. 1: A*. > 1: weighted A* (faster, may be longer).
// `trace` (optional) is called for every expansion with (cell, g, h).
inline PlanResult plan(const CostGrid& cg, Cell s, Cell goal, double hWeight,
                       const std::function<void(Cell, double, double)>& trace = nullptr)
{
    PlanResult res;
    if (cg.at(s.i, s.j) == kLethal || cg.at(goal.i, goal.j) == kLethal) { return res; }
    std::vector<double> g(kW * kH, kLethal);
    std::vector<int> parent(kW * kH, -1);
    std::vector<char> closed(kW * kH, 0);
    using Item = std::pair<double, int>;                     // (f, cell index)
    std::priority_queue<Item, std::vector<Item>, std::greater<Item>> open;
    const int si = s.j * kW + s.i;
    const int gi = goal.j * kW + goal.i;
    g[si] = 0.0;
    open.push({hWeight * octile(s, goal), si});
    while (!open.empty()) {
        const int cur = open.top().second;
        open.pop();
        if (closed[cur]) { continue; }                     // stale queue entry
        closed[cur] = 1;
        ++res.expanded;
        const Cell c{cur % kW, cur / kW};
        if (trace) { trace(c, g[cur], octile(c, goal)); }
        if (cur == gi) { break; }
        for (int dj = -1; dj <= 1; ++dj) {
            for (int di = -1; di <= 1; ++di) {
                if (di == 0 && dj == 0) { continue; }
                const Cell n{c.i + di, c.j + dj};
                const double cn = cg.at(n.i, n.j);
                if (cn == kLethal) { continue; }
                // no corner cutting: a diagonal step needs both side cells free
                if (di != 0 && dj != 0 && (cg.at(c.i + di, c.j) == kLethal || cg.at(c.i, c.j + dj) == kLethal)) {
                    continue;
                }
                const int ni = n.j * kW + n.i;
                const double step = (di != 0 && dj != 0 ? std::sqrt(2.0) : 1.0) * kCell * (1.0 + cn);
                if (g[cur] + step < g[ni]) {
                    g[ni] = g[cur] + step;
                    parent[ni] = cur;
                    open.push({g[ni] + hWeight * octile(n, goal), ni});
                }
            }
        }
    }
    if (!closed[gi]) { return res; }
    res.cost = g[gi];
    for (int k = gi; k != -1; k = parent[k]) { res.path.insert(res.path.begin(), Cell{k % kW, k / kW}); }
    return res;
}

inline Cell cellAt(double x, double y)
{
    return {static_cast<int>(std::floor(x / kCell)), static_cast<int>(std::floor(y / kCell))};
}

// Half-resolution drawing: '#' wall, '+' blocked by inflation, '*' path, '.' free.
inline void drawPlan(const Grid& g, const CostGrid& cg, const std::vector<Cell>& path)
{
    std::vector<char> onPath(kW * kH, 0);
    for (const Cell& c : path) { onPath[c.j * kW + c.i] = 1; }
    for (int j = kH - 1; j >= 0; j -= 2) {
        std::string row;
        for (int i = 0; i < kW; i += 2) {
            char ch = '.';
            for (int b = 0; b < 2; ++b) {
                for (int a = 0; a < 2; ++a) {
                    const int ii = i + a;
                    const int jj = j - b;
                    if (onPath[jj * kW + ii]) { ch = '*'; }
                    else if (ch != '*' && g.occ(ii, jj)) { ch = '#'; }
                    else if (ch == '.' && cg.at(ii, jj) == kLethal) { ch = '+'; }
                }
            }
            row += ch;
        }
        std::printf("|%s|\n", row.c_str());
    }
}

}  // namespace rb
