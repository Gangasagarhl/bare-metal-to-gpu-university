// pickplace.hpp - F9-57: the pick-and-place task as a sequence of planned stages.
// `attachInModel` = true is the correct pipeline; false reproduces the forensic fault
// (the planning scene is not told that the object is now held by the tool).
#pragma once
#include "arm.hpp"
#include <string>

namespace arm {

struct Stage { std::string name; std::vector<Q> path; };

inline void printQ(const char* label, const Q& q)
{
    const P2 t = fk(q)[3];
    std::printf("  %-10s q = (%6.1f, %6.1f, %6.1f) deg, tool tip (%.3f, %.3f)\n", label, q[0] * 180 / kPi,
                q[1] * 180 / kPi, q[2] * 180 / kPi, t.x, t.z);
}

inline int runPickPlace(bool attachInModel)
{
    Scene model;                         // what the planner believes
    rb::Rng rng(57);
    const int up = -1;                   // elbow-up IK branch, used for every target
    const double down = -kPi / 2;        // tool pointing straight down
    const Q home{kPi / 2, -kPi / 2, -kPi / 2};
    const P2 pick{0.62, 0.0};            // object bottom centre on the table
    const P2 place{0.28, 0.15};          // object bottom centre on the shelf top
    const std::optional<Q> preGrasp = ik(pick.x, kObjH + 0.10, down, up);
    const std::optional<Q> prePlace = ik(place.x, place.z + kObjH + 0.10, down, up);
    if (!preGrasp || !prePlace) { std::printf("IK failed\n"); return 2; }
    std::printf("IK solutions:\n");
    printQ("home", home);
    printQ("pre-grasp", *preGrasp);
    printQ("pre-place", *prePlace);

    std::vector<Stage> stages;
    auto planTo = [&](const char* name, const Q& from, const Q& to) {
        std::vector<Q> p = rrtConnect(model, from, to, rng);
        if (p.empty()) { return false; }
        p = shortcut(model, p, 100, rng);
        stages.push_back({name, p});
        return true;
    };
    bool ok = planTo("move to pre-grasp", home, *preGrasp);
    model.allowToolObject = true;                                    // the tool may touch the object
    const std::vector<Q> approach = cartesianZ(*preGrasp, -0.10, up);
    if (approach.empty()) { std::printf("Cartesian approach failed\n"); return 2; }
    stages.push_back({"approach (Cartesian, -0.10 m)", approach});
    if (attachInModel) { model.held = true; }                        // attach: object moves with the tool
    else { model.objectInModel = false; }                            // the fault: object forgotten
    const std::vector<Q> lift = cartesianZ(approach.back(), 0.10, up);
    if (lift.empty()) { std::printf("Cartesian lift failed\n"); return 2; }
    stages.push_back({"lift (Cartesian, +0.10 m)", lift});
    ok = ok && planTo("transfer to pre-place", lift.back(), *prePlace);
    const std::vector<Q> lower = cartesianZ(*prePlace, -0.10, up);
    if (lower.empty()) { std::printf("Cartesian lower failed\n"); return 2; }
    stages.push_back({"lower (Cartesian, -0.10 m)", lower});
    // detach: the object now rests on the shelf
    model.held = false;
    model.objectInModel = true;
    model.objectAt = place;
    const std::vector<Q> retreat = cartesianZ(lower.back(), 0.10, up);
    if (retreat.empty()) { std::printf("Cartesian retreat failed\n"); return 2; }
    stages.push_back({"retreat (Cartesian, +0.10 m)", retreat});
    model.allowToolObject = false;
    ok = ok && planTo("return home", retreat.back(), home);
    if (!ok) { std::printf("planning failed\n"); return 2; }

    // Execute in the simulated world, where the object really is carried between attach and
    // detach, and check every 0.01 rad of motion for collisions.
    std::printf("stage                            waypoints  joint travel(rad)  time(s)  world check\n");
    double total = 0.0;
    int collisions = 0;
    for (std::size_t s = 0; s < stages.size(); ++s) {
        Scene world;
        world.allowToolObject = s >= 1 && s <= 5;                    // approach ... retreat
        world.held = s >= 2 && s <= 4;                               // lift, transfer, lower
        world.objectAt = s >= 5 ? place : pick;
        const std::vector<Q>& p = stages[s].path;
        double travel = 0.0;
        std::string firstHit;
        for (std::size_t k = 1; k < p.size(); ++k) {
            travel += qdist(p[k - 1], p[k]);
            const int n = std::max(1, static_cast<int>(std::ceil(qdist(p[k - 1], p[k]) / 0.01)));
            for (int m = 0; m <= n && firstHit.empty(); ++m) {
                Q q;
                for (int j = 0; j < 3; ++j) { q[j] = p[k - 1][j] + (p[k][j] - p[k - 1][j]) * m / n; }
                firstHit = world.collision(q);
                if (!firstHit.empty()) {
                    const P2 t = fk(q)[3];
                    firstHit += " (tool tip at " + std::to_string(t.x).substr(0, 5) + ", " +
                                std::to_string(t.z).substr(0, 5) + ")";
                }
            }
        }
        const double d = duration(p);
        total += d;
        collisions += firstHit.empty() ? 0 : 1;
        std::printf("%-32s %9zu  %17.3f  %7.2f  %s\n", stages[s].name.c_str(), p.size(), travel, d,
                    firstHit.empty() ? "clear" : ("COLLISION: " + firstHit).c_str());
    }
    std::printf("total motion time %.2f s; stages with a collision in the world: %d\n", total, collisions);
    return collisions == 0 ? 0 : 3;
}

}  // namespace arm
