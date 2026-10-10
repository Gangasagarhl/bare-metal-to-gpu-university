// rrt.cpp - F9-55 lab: RRT, RRT with shortcutting, and RRT* on the living room -> hall
// query of F9-54, 10 random seeds each; A* on the 0.1 m grid is the reference.
#include "rrt.hpp"
#include <algorithm>

double median(std::vector<double> v)
{
    std::sort(v.begin(), v.end());
    return v.empty() ? 0.0 : v[v.size() / 2];
}

int main()
{
    const rb::Grid house = rb::makeHouse();
    const rb::CostGrid cg = rb::inflate(house, 0.20);
    const rb::Checker chk{&cg, 0.05};
    const rb::Pt start{1.0, 1.9};
    const rb::Pt goal{6.8, 4.8};
    const rb::PlanResult ref = rb::plan(cg, rb::cellAt(start.x, start.y), rb::cellAt(goal.x, goal.y), 1.0);
    std::printf("reference: A* on the grid, cost %.3f m\n", ref.cost);
    std::printf("seed | RRT: iters nodes length | +shortcut | RRT* (3000 iters): first-sol iter, length\n");
    std::vector<double> lr, ls, lstar, it1;
    rb::RrtResult drawn;
    for (int seed = 1; seed <= 10; ++seed) {
        rb::Rng rng(static_cast<std::uint64_t>(seed));
        rb::RrtOptions o;
        const rb::RrtResult a = rb::rrt(chk, start, goal, o, rng);
        const std::vector<rb::Pt> sc = rb::shortcut(chk, a.path, 200, rng);
        rb::RrtOptions os;
        os.star = true;
        os.stopAtFirst = false;
        rb::Rng rng2(static_cast<std::uint64_t>(seed));
        const rb::RrtResult b = rb::rrt(chk, start, goal, os, rng2);
        std::printf("%4d | %5d %5d %6.3f | %6.3f | %5d %6.3f\n", seed, a.firstSolutionIter, a.nodes, a.cost,
                    rb::length(sc), b.firstSolutionIter, b.cost);
        if (!a.path.empty()) { lr.push_back(a.cost); ls.push_back(rb::length(sc)); it1.push_back(a.firstSolutionIter); }
        if (!b.path.empty()) { lstar.push_back(b.cost); }
        if (seed == 1) { drawn = a; }
    }
    std::printf("solved: RRT %zu/10, RRT* %zu/10\n", lr.size(), lstar.size());
    std::printf("median length: RRT %.3f m, RRT+shortcut %.3f m, RRT* %.3f m (A* grid %.3f m)\n",
                median(lr), median(ls), median(lstar), ref.cost);
    std::printf("median iterations to first solution (RRT): %.0f\n", median(it1));
    // draw seed 1's tree: ':' cells crossed by tree edges, '*' the path, '#' walls
    std::vector<char> mark(rb::kW * rb::kH, 0);
    auto paint = [&](rb::Pt a, rb::Pt b, char c) {
        for (int k = 0; k <= 20; ++k) {
            const rb::Cell cell = rb::cellAt(a.x + (b.x - a.x) * k / 20, a.y + (b.y - a.y) * k / 20);
            char& m = mark[cell.j * rb::kW + cell.i];
            if (m != '*') { m = c; }
        }
    };
    for (std::size_t k = 1; k < drawn.tree.pt.size(); ++k) { paint(drawn.tree.pt[drawn.tree.parent[k]], drawn.tree.pt[k], ':'); }
    for (std::size_t k = 1; k < drawn.path.size(); ++k) { paint(drawn.path[k - 1], drawn.path[k], '*'); }
    std::printf("seed 1 RRT tree (%d nodes) and path, each character = 2 x 2 cells:\n", drawn.nodes);
    for (int j = rb::kH - 1; j >= 0; j -= 2) {
        std::string row;
        for (int i = 0; i < rb::kW; i += 2) {
            char ch = '.';
            for (int b = 0; b < 2; ++b) {
                for (int a = 0; a < 2; ++a) {
                    const char m = mark[(j - b) * rb::kW + i + a];
                    if (m == '*') { ch = '*'; }
                    else if (m == ':' && ch != '*') { ch = ':'; }
                    else if (ch == '.' && house.occ(i + a, j - b)) { ch = '#'; }
                }
            }
            row += ch;
        }
        std::printf("|%s|\n", row.c_str());
    }
    return 0;
}
