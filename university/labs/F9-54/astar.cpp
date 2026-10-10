// astar.cpp - F9-54 lab: plan through the house with Dijkstra, A* and weighted A*.
// The robot is a disc of radius 0.20 m, so the walls are inflated by 0.20 m first.
#include "planner.hpp"

int main()
{
    const rb::Grid house = rb::makeHouse();
    const rb::CostGrid cg = rb::inflate(house, 0.20);
    struct Query { const char* name; double sx, sy, gx, gy; };
    const Query queries[] = {{"living room -> hall", 1.0, 1.9, 6.8, 4.8},
                             {"bedroom -> kitchen corner", 3.2, 5.0, 7.5, 0.5}};
    struct Algo { const char* name; double w; };
    const Algo algos[] = {{"Dijkstra (w=0)", 0.0}, {"A* (w=1)", 1.0}, {"weighted A* (w=2)", 2.0},
                          {"weighted A* (w=5)", 5.0}};
    rb::PlanResult shown;
    for (const Query& q : queries) {
        const rb::Cell s = rb::cellAt(q.sx, q.sy);
        const rb::Cell g = rb::cellAt(q.gx, q.gy);
        std::printf("%s: start cell (%d,%d), goal cell (%d,%d), straight line %.2f m\n", q.name, s.i, s.j,
                    g.i, g.j, std::hypot(q.gx - q.sx, q.gy - q.sy));
        for (const Algo& a : algos) {
            const rb::PlanResult r = rb::plan(cg, s, g, a.w);
            std::printf("  %-18s cost %.3f m  expanded %5d  path cells %3zu\n", a.name, r.cost, r.expanded,
                        r.path.size());
            if (&q == &queries[0] && a.w == 1.0) { shown = r; }
        }
    }
    std::printf("A* path, living room -> hall (each character = 2 x 2 cells):\n");
    rb::drawPlan(house, cg, shown.path);
    return 0;
}
