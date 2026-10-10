// heuristic_bug.cpp - F9-54 forensic evidence: "the long way to the kitchen".
// The new planner release (v2, A*) is compared with the old one (v1, Dijkstra) on two
// delivery requests. v2 prints a debug trace of its first expansions.
// The fault (explained in the answer key): v2's heuristic was computed in millimetres
// while step costs are in metres, i.e. it is 1000 times too large.
#include "planner.hpp"

int main()
{
    const rb::Grid house = rb::makeHouse();
    const rb::CostGrid cg = rb::inflate(house, 0.20);
    const double hScale = 1000.0;              // the fault: "distance in mm" into a metre-based A*
    struct Request { const char* name; double sx, sy, gx, gy; };
    const Request reqs[] = {{"request 17: bedroom -> hall cabinet", 3.2, 5.0, 6.8, 4.8},
                            {"request 18: living room -> kitchen door", 1.0, 1.9, 5.0, 3.0}};
    for (const Request& q : reqs) {
        const rb::Cell s = rb::cellAt(q.sx, q.sy);
        const rb::Cell g = rb::cellAt(q.gx, q.gy);
        std::printf("%s\n", q.name);
        int shown = 0;
        const rb::PlanResult v2 = rb::plan(cg, s, g, hScale, [&](rb::Cell c, double gc, double h) {
            if (shown++ < 4) {
                std::printf("  v2 debug: expand (%2d,%2d) g=%.3f h=%.1f\n", c.i, c.j, gc, hScale * h);
            }
        });
        const rb::PlanResult v1 = rb::plan(cg, s, g, 0.0);
        std::printf("  v1 (Dijkstra): cost %.3f m, expanded %4d\n", v1.cost, v1.expanded);
        std::printf("  v2 (A*):       cost %.3f m, expanded %4d\n", v2.cost, v2.expanded);
        if (&q == &reqs[1]) {
            std::printf("  v2 path:\n");
            rb::drawPlan(house, cg, v2.path);
        }
    }
    return 0;
}
