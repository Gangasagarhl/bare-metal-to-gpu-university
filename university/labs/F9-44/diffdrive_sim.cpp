// diffdrive_sim.cpp - the university's tiny 2D robot simulator with sensor topics (F9-44).
// Not Gazebo. A differential-drive robot in a 4 m x 3 m room with one box, a fixed physics
// step, a simulated clock, a real-time factor, an 8-beam range sensor, wheel odometry with
// a small left-wheel slip, ground truth, a safety stop, and a teleop script read from stdin.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <numbers>
#include <string>
#include <vector>

struct Pose { double x = 0.0, y = 0.0, th = 0.0; };
struct Rect { double x0, y0, x1, y1; };

// Distance from (x, y) along angle a to the first wall or box face (slab method).
double rayCast(double x, double y, double a, const std::vector<Rect>& boxes, double maxRange)
{
    double best = maxRange;
    double dx = std::cos(a), dy = std::sin(a);
    auto hitSegment = [&](double t, double px, double py, const Rect& r) {
        if (t > 1e-9 && t < best && px >= r.x0 - 1e-9 && px <= r.x1 + 1e-9 && py >= r.y0 - 1e-9 && py <= r.y1 + 1e-9) best = t;
    };
    for (const auto& r : boxes) {
        for (double bx : {r.x0, r.x1})
            if (std::fabs(dx) > 1e-12) { double t = (bx - x) / dx; hitSegment(t, bx, y + t * dy, r); }
        for (double by : {r.y0, r.y1})
            if (std::fabs(dy) > 1e-12) { double t = (by - y) / dy; hitSegment(t, x + t * dx, by, r); }
    }
    return best;
}

int main()
{
    const double room[4] = {0.0, 0.0, 4.0, 3.0};
    // the room's four walls as thin boxes, then one obstacle box
    const std::vector<Rect> boxes = {{room[0], room[1], room[2], room[1]}, {room[0], room[3], room[2], room[3]},
                                     {room[0], room[1], room[0], room[3]}, {room[2], room[1], room[2], room[3]},
                                     {2.5, 1.2, 3.0, 1.8}};
    const double stepS = 0.010;            // physics step: 10 ms of simulated time
    const double wheelBase = 0.30, slipLeft = 0.98;   // left wheel delivers 98 % of its command
    double rtf = 1.0;                      // real-time factor: simulated seconds per wall second
    std::string clock = "sim";             // the controller times its commands with this clock
    struct Cmd { double v, w, seconds; };
    std::vector<Cmd> script;
    std::string word;
    while (std::cin >> word) {
        if (word == "rtf") std::cin >> rtf;
        else if (word == "clock") std::cin >> clock;
        else if (word == "drive") { Cmd c; std::cin >> c.v >> c.w >> c.seconds; script.push_back(c); }
    }
    std::printf("world: room 4.0 x 3.0 m, box x 2.5..3.0 y 1.2..1.8; step %.0f ms; rtf %.2f; controller clock: %s\n",
                stepS * 1000, rtf, clock.c_str());
    Pose truth{0.5, 1.5, 0.0}, odom = truth;
    double simT = 0.0, wallT = 0.0;
    bool stopped = false;
    long step = 0;
    for (const auto& c : script) {
        std::printf("teleop: v=%.2f m/s w=%.2f rad/s for %.2f s (%s time)\n", c.v, c.w, c.seconds, clock.c_str());
        double startSim = simT, startWall = wallT;
        while ((clock == "sim" ? simT - startSim : wallT - startWall) < c.seconds - 1e-9) {
            double front = rayCast(truth.x, truth.y, truth.th, boxes, 5.0);
            double v = c.v;
            if (v > 0.0 && front < 0.35) {             // safety stop: never drive into an obstacle
                v = 0.0;
                if (!stopped) std::printf("  t=%6.2f safety stop: front range %.3f m < 0.35 m\n", simT, front);
                stopped = true;
            } else {
                stopped = false;
            }
            // wheel speeds from (v, w); the left wheel slips a little in the "real" world
            double vl = (v - c.w * wheelBase / 2) * slipLeft, vr = v + c.w * wheelBase / 2;
            double vTrue = (vl + vr) / 2, wTrue = (vr - vl) / wheelBase;
            truth.x += vTrue * std::cos(truth.th) * stepS;
            truth.y += vTrue * std::sin(truth.th) * stepS;
            truth.th += wTrue * stepS;
            // odometry believes the wheels did exactly what was commanded
            odom.x += v * std::cos(odom.th) * stepS;
            odom.y += v * std::sin(odom.th) * stepS;
            odom.th += c.w * stepS;
            simT += stepS; wallT += stepS / rtf; ++step;
            if (step % 50 == 0) {                         // print every 0.5 s of simulated time
                std::printf("  t=%6.2f wall=%6.2f truth (%.2f, %.2f, %4.0f deg) odom (%.2f, %.2f, %4.0f deg) scan:",
                            simT, wallT, truth.x, truth.y, truth.th * 180 / std::numbers::pi, odom.x, odom.y,
                            odom.th * 180 / std::numbers::pi);
                for (int b = 0; b < 8; ++b)
                    std::printf(" %.2f", rayCast(truth.x, truth.y, truth.th + b * std::numbers::pi / 4, boxes, 5.0));
                std::printf("\n");
            }
        }
    }
    std::printf("end: sim %.2f s, wall %.2f s; truth (%.3f, %.3f); odom (%.3f, %.3f); odom error %.3f m\n", simT, wallT,
                truth.x, truth.y, odom.x, odom.y, std::hypot(odom.x - truth.x, odom.y - truth.y));
    return 0;
}
