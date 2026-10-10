// thin_wall.cpp - F9-55 forensic evidence: "the shortcut through the wall".
// The planner service runs RRT, then shortcutting, then hands the path to the controller.
// An independent validator (a dense 1 cm check on the same inflated grid) checks every
// segment afterwards. Prints the configuration, the paths and the validator's report.
#include "rrt.hpp"

int main()
{
    const rb::Grid house = rb::makeHouse();
    const rb::CostGrid cg = rb::inflate(house, 0.20);
    const rb::Checker planner{&cg, 0.60};       // the service's collision checker
    const rb::Checker validator{&cg, 0.01};     // the independent validator
    rb::RrtOptions o;
    std::printf("planner config: robot_radius 0.20, step %.2f, goal_bias %.2f, goal_tol %.2f,"
                " max_iter %d, edge_check_resolution %.2f, shortcut_tries 200\n",
                o.step, o.goalBias, o.goalTol, o.maxIter, planner.edgeStep);
    const rb::Pt start{1.0, 1.9};
    const rb::Pt goal{6.8, 4.8};
    int runsBad = 0;
    for (int seed = 1; seed <= 25; ++seed) {               // the service log of 25 requests
        rb::Rng rs(static_cast<std::uint64_t>(seed));
        const rb::RrtResult rr = rb::rrt(planner, start, goal, o, rs);
        const std::vector<rb::Pt> ss = rb::shortcut(planner, rr.path, 200, rs);
        for (std::size_t k = 1; k < ss.size(); ++k) {
            if (!validator.edge(ss[k - 1], ss[k])) { ++runsBad; break; }
        }
    }
    std::printf("25 requests living room -> hall: validator rejected %d final paths\n", runsBad);
    std::printf("request with seed 9 in detail:\n");
    rb::Rng rng(9);
    const rb::RrtResult r = rb::rrt(planner, start, goal, o, rng);
    const std::vector<rb::Pt> sc = rb::shortcut(planner, r.path, 200, rng);
    auto check = [&](const char* name, const std::vector<rb::Pt>& p) {
        int bad = 0;
        for (std::size_t k = 1; k < p.size(); ++k) { bad += validator.edge(p[k - 1], p[k]) ? 0 : 1; }
        std::printf("%s: %zu waypoints, length %.3f m, validator: %d invalid segment(s)\n", name, p.size(),
                    rb::length(p), bad);
    };
    check("raw RRT path", r.path);
    check("after shortcutting", sc);
    std::printf("after shortcutting, segment by segment:\n");
    for (std::size_t k = 1; k < sc.size(); ++k) {
        const double len = rb::dist(sc[k - 1], sc[k]);
        std::printf("  %2zu: (%.2f, %.2f) -> (%.2f, %.2f)  %.2f m  %s\n", k, sc[k - 1].x, sc[k - 1].y, sc[k].x,
                    sc[k].y, len, validator.edge(sc[k - 1], sc[k]) ? "ok" : "INVALID");
        int inBand = 0;
        int inWall = 0;
        for (double f = 0.0; f <= 1.0; f += 0.01 / len) {   // the 1 cm points that failed
            const rb::Pt p{sc[k - 1].x + (sc[k].x - sc[k - 1].x) * f, sc[k - 1].y + (sc[k].y - sc[k - 1].y) * f};
            if (validator.point(p)) { continue; }
            if (house.occAt(p.x, p.y)) { ++inWall; } else { ++inBand; }
            if (inBand + inWall == 1) { std::printf("      first failing point (%.2f, %.2f)\n", p.x, p.y); }
        }
        if (inBand + inWall > 0) {
            std::printf("      failing 1 cm points: %d inside the inflation band, %d inside a wall\n", inBand, inWall);
        }
    }
    return 0;
}
