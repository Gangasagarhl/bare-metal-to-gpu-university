// nav.hpp - F9-56: the university's mini navigation stack, built to show the PARTS that a
// navigation framework such as Nav2 has (costmap layers, global planner, controller,
// progress and goal checkers, recovery). It is our own code, not Nav2 and not its API.
#pragma once
#include "../F9-54/planner.hpp"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace rb {

struct NavConfig
{
    double robotRadius = 0.20;        // radius the planner assumes (metres)
    double inflationRadius = 0.60;    // cost decays to zero at this distance from obstacles
    double costScaling = 6.0;         // how fast the cost decays beyond robotRadius (1/m)
    double maxVel = 0.40;             // m/s
    double maxRotVel = 1.00;          // rad/s
    double lookahead = 0.50;          // pure pursuit look-ahead distance (m)
    double xyTol = 0.15;              // goal checker (m)
    double yawTol = 0.20;             // goal checker (rad)
    double progressTimeout = 4.0;     // s without 0.10 m of progress -> recovery
    double controlRate = 20.0;        // Hz
    std::vector<Pose> goals;
    std::vector<Rect> unmapped;       // obstacles in the world that the map does not have
    std::vector<double> appearAt;     // ... and the time (s) at which each one is put down
};

inline NavConfig readConfig(std::istream& in)
{
    NavConfig c;
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream s(line);
        std::string key;
        if (!(s >> key) || key[0] == '#') { continue; }
        if (key == "goal") { Pose p; s >> p.x >> p.y >> p.th; c.goals.push_back(p); }
        else if (key == "unmapped_box") {
            Rect r{};
            double at = 0.0;
            s >> at >> r.x0 >> r.y0 >> r.x1 >> r.y1;
            c.unmapped.push_back(r);
            c.appearAt.push_back(at);
        }
        else if (key == "robot_radius") { s >> c.robotRadius; }
        else if (key == "inflation_radius") { s >> c.inflationRadius; }
        else if (key == "cost_scaling") { s >> c.costScaling; }
        else if (key == "max_vel") { s >> c.maxVel; }
        else if (key == "max_rot_vel") { s >> c.maxRotVel; }
        else if (key == "lookahead") { s >> c.lookahead; }
        else if (key == "xy_goal_tolerance") { s >> c.xyTol; }
        else if (key == "yaw_goal_tolerance") { s >> c.yawTol; }
        else if (key == "progress_timeout") { s >> c.progressTimeout; }
        else if (key == "control_rate") { s >> c.controlRate; }
        else { std::printf("config: unknown key '%s' ignored\n", key.c_str()); }
    }
    return c;
}

// Costmap = static layer (the map) + obstacle layer (from LiDAR) + inflation.
class Costmap
{
public:
    Costmap(const Grid& staticMap, const NavConfig& c) : static_(staticMap), cfg_(c), obstacle_(kW * kH, 0) {}

    // Obstacle layer: mark beam end points, clear cells the beams passed through.
    void addScan(const Pose& p, const std::vector<double>& z, double maxRange)
    {
        const int n = static_cast<int>(z.size());
        for (int k = 0; k < n; ++k) {
            const double a = p.th + 2.0 * kPi * k / n;
            const double r = std::fmin(z[k], maxRange);
            for (double d = 0.0; d < r - 0.05; d += 0.05) {       // clearing
                const Cell c = cellAt(p.x + d * std::cos(a), p.y + d * std::sin(a));
                if (static_.inside(c.i, c.j)) { obstacle_[c.j * kW + c.i] = 0; }
            }
            if (z[k] < maxRange) {                                // marking
                const Cell c = cellAt(p.x + r * std::cos(a), p.y + r * std::sin(a));
                if (static_.inside(c.i, c.j) && !static_.occ(c.i, c.j)) { obstacle_[c.j * kW + c.i] = 1; }
            }
        }
    }
    bool lethalSource(int i, int j) const { return static_.occ(i, j) || obstacle_[j * kW + i] != 0; }

    // Inflation: lethal within robotRadius, then a decaying extra cost up to inflationRadius.
    CostGrid build() const
    {
        CostGrid cg;
        const int r = static_cast<int>(std::ceil(cfg_.inflationRadius / kCell));
        for (int j = 0; j < kH; ++j) {
            for (int i = 0; i < kW; ++i) {
                if (!lethalSource(i, j)) { continue; }
                for (int b = -r; b <= r; ++b) {
                    for (int a = -r; a <= r; ++a) {
                        if (!static_.inside(i + a, j + b)) { continue; }
                        const double d = std::hypot(a, b) * kCell;
                        double& c = cg.cost[(j + b) * kW + (i + a)];
                        if (d <= cfg_.robotRadius) { c = kLethal; }
                        else if (d <= cfg_.inflationRadius) {
                            c = std::fmax(c, 3.0 * std::exp(-cfg_.costScaling * (d - cfg_.robotRadius)));
                        }
                    }
                }
            }
        }
        return cg;
    }
private:
    const Grid& static_;
    NavConfig cfg_;
    std::vector<std::uint8_t> obstacle_;
};

// Pure pursuit: steer along an arc through the path point `lookahead` metres ahead.
struct Cmd { double v, w; };

inline Cmd purePursuit(const Pose& p, const std::vector<Pose>& path, std::size_t& idx, const NavConfig& c)
{
    while (idx + 1 < path.size() && std::hypot(path[idx].x - p.x, path[idx].y - p.y) < c.lookahead) { ++idx; }
    const Pose& t = path[idx];
    const double dx = t.x - p.x;
    const double dy = t.y - p.y;
    const double yr = -std::sin(p.th) * dx + std::cos(p.th) * dy;   // target in the robot frame
    const double xr = std::cos(p.th) * dx + std::sin(p.th) * dy;
    const double ang = std::atan2(yr, xr);
    if (std::fabs(ang) > 1.0) {                                      // facing away: turn on the spot
        return {0.0, std::copysign(c.maxRotVel, ang)};
    }
    const double l2 = dx * dx + dy * dy;
    const double v = c.maxVel;
    const double w = std::clamp(v * 2.0 * yr / std::fmax(l2, 1e-6), -c.maxRotVel, c.maxRotVel);
    return {v, w};
}

// The simulated robot: a disc of 0.20 m radius (the TRUE body, whatever the config says).
constexpr double kBodyRadius = 0.20;

inline bool touches(const Grid& world, const Pose& p)
{
    const Cell c = cellAt(p.x, p.y);
    for (int b = -3; b <= 3; ++b) {
        for (int a = -3; a <= 3; ++a) {
            const int i = c.i + a;
            const int j = c.j + b;
            if (!world.occ(i, j)) { continue; }
            const double nx = std::clamp(p.x, i * kCell, (i + 1) * kCell);   // nearest point of the cell
            const double ny = std::clamp(p.y, j * kCell, (j + 1) * kCell);
            if (std::hypot(nx - p.x, ny - p.y) < kBodyRadius) { return true; }
        }
    }
    return false;
}

inline std::vector<Pose> toPoses(const std::vector<Cell>& cells)
{
    std::vector<Pose> out;
    for (const Cell& c : cells) { out.push_back({(c.i + 0.5) * kCell, (c.j + 0.5) * kCell, 0.0}); }
    return out;
}

// Run every goal of the config in turn; print an event log. Returns the number of failures.
inline int runNavigation(std::istream& in)
{
    const NavConfig cfg = readConfig(in);
    const Grid map = makeHouse();                    // what the robot was given
    Grid world = makeHouse();                        // what is really there (changes over time)
    std::size_t placed = 0;
    LidarSpec lidar;
    lidar.beams = 180;
    Rng rng(56);
    Costmap costmap(map, cfg);
    const double dt = 1.0 / cfg.controlRate;
    Pose robot{1.0, 1.9, 0.0};
    double t = 0.0;
    int failures = 0;
    int collisionsTotal = 0;
    std::printf("config: robot_radius %.2f inflation_radius %.2f max_vel %.2f lookahead %.2f"
                " xy_tol %.2f yaw_tol %.2f\n", cfg.robotRadius, cfg.inflationRadius, cfg.maxVel,
                cfg.lookahead, cfg.xyTol, cfg.yawTol);
    for (std::size_t g = 0; g < cfg.goals.size(); ++g) {
        const Pose goal = cfg.goals[g];
        std::printf("t=%6.2f goal %zu (%.2f, %.2f, %.2f) accepted\n", t, g + 1, goal.x, goal.y, goal.th);
        const double tStart = t;
        double travelled = 0.0;
        int recoveries = 0;
        int collisions = 0;
        std::vector<Pose> path;
        std::size_t idx = 0;
        double nextPlan = t;
        double nextScan = t;
        Pose progressPose = robot;
        double progressTime = t;
        bool done = false;
        bool inContact = false;
        while (!done) {
            if (placed < cfg.unmapped.size() && t >= cfg.appearAt[placed]) {   // someone puts a box down
                ++placed;
                world = makeHouse(std::vector<Rect>(cfg.unmapped.begin(), cfg.unmapped.begin() + static_cast<long>(placed)));
                std::printf("t=%6.2f   (world: a box not in the map appears at x %.1f-%.1f, y %.1f-%.1f)\n", t,
                            cfg.unmapped[placed - 1].x0, cfg.unmapped[placed - 1].x1, cfg.unmapped[placed - 1].y0,
                            cfg.unmapped[placed - 1].y1);
            }
            if (t >= nextScan) {                                         // sensors at 5 Hz
                costmap.addScan(robot, scan(world, robot, lidar, rng), lidar.maxRange);
                nextScan += 0.2;
            }
            bool planFailed = false;
            if (t >= nextPlan) {                                         // global planner at 1 Hz
                const CostGrid cg = costmap.build();
                const PlanResult pr = plan(cg, cellAt(robot.x, robot.y), cellAt(goal.x, goal.y), 1.0);
                if (pr.path.empty()) {
                    std::printf("t=%6.2f   planner: no path from (%.2f, %.2f)\n", t, robot.x, robot.y);
                    planFailed = true;
                } else {
                    bool blocked = false;                                // old path now through a lethal cell?
                    for (std::size_t k = idx; k < path.size(); ++k) {
                        const Cell c = cellAt(path[k].x, path[k].y);
                        if (cg.at(c.i, c.j) == kLethal) { blocked = true; }
                    }
                    if (path.empty()) {
                        std::printf("t=%6.2f   planner: path of %zu cells, cost %.2f\n", t, pr.path.size(), pr.cost);
                    } else if (blocked) {
                        std::printf("t=%6.2f   planner: old path blocked by the obstacle layer; new path of %zu"
                                    " cells, cost %.2f\n", t, pr.path.size(), pr.cost);
                    }
                    path = toPoses(pr.path);
                    idx = 0;
                }
                nextPlan += 1.0;
            }
            const double dGoal = std::hypot(goal.x - robot.x, goal.y - robot.y);
            Cmd cmd{0.0, 0.0};
            if (dGoal <= cfg.xyTol) {                                    // goal checker: position reached
                const double e = wrapAngle(goal.th - robot.th);
                if (std::fabs(e) <= cfg.yawTol) {
                    std::printf("t=%6.2f goal %zu SUCCEEDED in %.1f s, travelled %.2f m, recoveries %d,"
                                " collisions %d\n", t, g + 1, t - tStart, travelled, recoveries, collisions);
                    done = true;
                    continue;
                }
                cmd = {0.0, std::copysign(std::fmin(cfg.maxRotVel, 2.0 * std::fabs(e)), e)};
            } else if (!path.empty()) {
                cmd = purePursuit(robot, path, idx, cfg);
                cmd.v = std::fmin(cmd.v, std::fmax(0.08, dGoal));        // slow down near the goal
            }
            // progress checker or planner failure -> recovery: back up 0.2 m, clear the plan, replan now
            if (std::hypot(robot.x - progressPose.x, robot.y - progressPose.y) >= 0.10) {
                progressPose = robot;
                progressTime = t;
            } else if (planFailed || (t - progressTime > cfg.progressTimeout && dGoal > cfg.xyTol)) {
                ++recoveries;
                std::printf("t=%6.2f   %s at (%.2f, %.2f) -> recovery %d: back up 0.2 m\n", t,
                            planFailed ? "planner failed" : "progress checker: no progress", robot.x, robot.y,
                            recoveries);
                for (int k = 0; k < 10; ++k) {
                    const Pose back{robot.x - 0.02 * std::cos(robot.th), robot.y - 0.02 * std::sin(robot.th), robot.th};
                    if (!touches(world, back) || touches(world, robot)) { robot = back; }
                    t += 0.05;
                }
                path.clear();
                nextPlan = t;
                progressPose = robot;
                progressTime = t;
                if (recoveries > 3) {
                    std::printf("t=%6.2f goal %zu ABORTED after %d recoveries\n", t, g + 1, recoveries);
                    ++failures;
                    done = true;
                }
                continue;
            }
            // move the simulated robot (unicycle model); a bumper stops it on contact
            const Pose next{robot.x + cmd.v * std::cos(robot.th) * dt, robot.y + cmd.v * std::sin(robot.th) * dt,
                            wrapAngle(robot.th + cmd.w * dt)};
            if (touches(world, next) && !touches(world, robot)) {
                if (!inContact) {
                    ++collisions;
                    std::printf("t=%6.2f   BUMPER: contact at (%.2f, %.2f), heading %.2f\n", t, next.x, next.y, next.th);
                }
                inContact = true;
                robot.th = next.th;                                      // it may still turn on the spot
            } else {
                inContact = false;
                travelled += std::hypot(next.x - robot.x, next.y - robot.y);
                robot = next;
            }
            t += dt;
            if (t - tStart > 120.0) {
                std::printf("t=%6.2f goal %zu ABORTED: time limit\n", t, g + 1);
                ++failures;
                done = true;
            }
        }
        collisionsTotal += collisions;
    }
    std::printf("summary: %zu goals, %d failed, %d bumper contacts\n", cfg.goals.size(), failures, collisionsTotal);
    return failures;
}

}  // namespace rb
