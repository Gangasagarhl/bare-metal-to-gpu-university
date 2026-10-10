// rrt.hpp - F9-55: RRT and RRT* for a disc robot in the house (continuous positions).
// Collision checking uses the inflated grid of F9-54: a point is valid if its cell is not
// blocked; an edge is valid if points every `edgeStep` metres along it are valid.
#pragma once
#include "../F9-54/planner.hpp"
#include <algorithm>
#include <vector>

namespace rb {

struct Pt { double x, y; };

struct Checker
{
    const CostGrid* cg;
    double edgeStep;        // metres between checked points on an edge
    mutable long checks = 0;
    bool point(Pt p) const
    {
        ++checks;
        return cg->at(static_cast<int>(std::floor(p.x / kCell)), static_cast<int>(std::floor(p.y / kCell))) != kLethal;
    }
    bool edge(Pt a, Pt b) const
    {
        const double len = std::hypot(b.x - a.x, b.y - a.y);
        const int n = std::max(1, static_cast<int>(std::ceil(len / edgeStep)));
        for (int k = 0; k <= n; ++k) {
            if (!point({a.x + (b.x - a.x) * k / n, a.y + (b.y - a.y) * k / n})) { return false; }
        }
        return true;
    }
};

struct Tree
{
    std::vector<Pt> pt;
    std::vector<int> parent;
    std::vector<double> cost;   // path length from the root
    std::vector<std::vector<int>> children;
};

struct RrtOptions
{
    double step = 0.3;          // maximum edge length (metres)
    double goalBias = 0.05;     // probability of sampling the goal itself
    double goalTol = 0.3;       // a node this close to the goal (and connectable) solves it
    int maxIter = 3000;
    bool star = false;          // RRT* : choose the best parent and rewire neighbours
    double radius = 0.8;        // RRT* neighbourhood radius (metres)
    bool stopAtFirst = true;    // plain RRT stops at the first solution
};

struct RrtResult
{
    std::vector<Pt> path;
    double cost = 0.0;
    int firstSolutionIter = -1;
    int nodes = 0;
    Tree tree;
};

inline double dist(Pt a, Pt b) { return std::hypot(a.x - b.x, a.y - b.y); }

inline RrtResult rrt(const Checker& chk, Pt start, Pt goal, const RrtOptions& o, Rng& rng)
{
    RrtResult res;
    Tree& t = res.tree;
    t.pt.push_back(start);
    t.parent.push_back(-1);
    t.cost.push_back(0.0);
    t.children.emplace_back();
    int goalNode = -1;
    for (int it = 1; it <= o.maxIter; ++it) {
        const Pt q = rng.uniform() < o.goalBias ? goal
                                                : Pt{rng.uniform() * kW * kCell, rng.uniform() * kH * kCell};
        int near = 0;                                         // nearest node (linear search)
        for (std::size_t k = 1; k < t.pt.size(); ++k) {
            if (dist(t.pt[k], q) < dist(t.pt[near], q)) { near = static_cast<int>(k); }
        }
        const double d = dist(t.pt[near], q);
        const Pt nw = d <= o.step ? q : Pt{t.pt[near].x + (q.x - t.pt[near].x) * o.step / d,
                                          t.pt[near].y + (q.y - t.pt[near].y) * o.step / d};
        if (!chk.point(nw) || !chk.edge(t.pt[near], nw)) { continue; }
        int par = near;
        double best = t.cost[near] + dist(t.pt[near], nw);
        std::vector<int> nbrs;
        if (o.star) {                                         // choose the cheapest valid parent
            for (std::size_t k = 0; k < t.pt.size(); ++k) {
                if (dist(t.pt[k], nw) <= o.radius) { nbrs.push_back(static_cast<int>(k)); }
            }
            for (int k : nbrs) {
                const double c = t.cost[k] + dist(t.pt[k], nw);
                if (c < best && chk.edge(t.pt[k], nw)) { best = c; par = k; }
            }
        }
        const int id = static_cast<int>(t.pt.size());
        t.pt.push_back(nw);
        t.parent.push_back(par);
        t.cost.push_back(best);
        t.children.emplace_back();
        t.children[par].push_back(id);
        if (o.star) {                                         // rewire: go through the new node if cheaper
            for (int k : nbrs) {
                const double c = best + dist(nw, t.pt[k]);
                if (c < t.cost[k] && chk.edge(nw, t.pt[k])) {
                    const double delta = t.cost[k] - c;
                    std::vector<int>& old = t.children[t.parent[k]];
                    old.erase(std::find(old.begin(), old.end(), k));
                    t.parent[k] = id;
                    t.children[id].push_back(k);
                    std::vector<int> stack = {k};              // lower the cost of k's subtree
                    while (!stack.empty()) {
                        const int m = stack.back();
                        stack.pop_back();
                        t.cost[m] -= delta;
                        for (int ch : t.children[m]) { stack.push_back(ch); }
                    }
                }
            }
        }
        if (dist(nw, goal) <= o.goalTol && chk.edge(nw, goal)) {
            if (goalNode == -1) { res.firstSolutionIter = it; }
            if (goalNode == -1 || t.cost[id] + dist(nw, goal) < t.cost[goalNode] + dist(t.pt[goalNode], goal)) {
                goalNode = id;
            }
            if (o.stopAtFirst) { break; }
        }
    }
    res.nodes = static_cast<int>(t.pt.size());
    if (goalNode == -1) { return res; }
    res.path.push_back(goal);
    for (int k = goalNode; k != -1; k = t.parent[k]) { res.path.insert(res.path.begin(), t.pt[k]); }
    for (std::size_t k = 1; k < res.path.size(); ++k) { res.cost += dist(res.path[k - 1], res.path[k]); }
    return res;
}

// Shortcutting: repeatedly try to join two random points of the path with a straight edge.
inline std::vector<Pt> shortcut(const Checker& chk, std::vector<Pt> p, int tries, Rng& rng)
{
    for (int t = 0; t < tries && p.size() > 2; ++t) {
        const std::size_t a = static_cast<std::size_t>(rng.uniform() * static_cast<double>(p.size()));
        const std::size_t b = static_cast<std::size_t>(rng.uniform() * static_cast<double>(p.size()));
        const std::size_t lo = std::min(a, b);
        const std::size_t hi = std::max(a, b);
        if (hi <= lo + 1) { continue; }
        if (chk.edge(p[lo], p[hi])) { p.erase(p.begin() + static_cast<long>(lo) + 1, p.begin() + static_cast<long>(hi)); }
    }
    return p;
}

inline double length(const std::vector<Pt>& p)
{
    double c = 0.0;
    for (std::size_t k = 1; k < p.size(); ++k) { c += dist(p[k - 1], p[k]); }
    return c;
}

}  // namespace rb
