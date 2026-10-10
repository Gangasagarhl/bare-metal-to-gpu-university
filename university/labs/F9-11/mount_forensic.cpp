// F9-11 forensic generator: the robot takes scans at three poses and its software puts
// every scan point into the world frame with its odometry pose and its configured LiDAR
// mount. For each scan it fits the right wall (a line of constant x) and the top wall
// (constant y) by averaging the points near them. The fault is described in the key only.
#include <cmath>
#include <cstdio>
#include <numbers>

#include "scan_sim.h"

int main()
{
    const ScanSpec spec;
    const double trueMountX = 0.20;     // where the LiDAR really is, ahead of the centre
    const double configMountX = 0.00;   // what the robot's configuration says
    struct Pose
    {
        const char* what;
        double x, y, yawDeg;
    };
    const Pose poses[] = {{"scan 1: start", 0.5, 0.3, 0.0},
                          {"scan 2: after driving 0.5 m straight", 1.0, 0.3, 0.0},
                          {"scan 3: after turning 90 deg left in place", 1.0, 0.3, 90.0}};
    std::printf("configured mount x = %.2f m\n", configMountX);
    std::printf("%-44s %16s %14s %10s\n", "scan (odometry pose)", "right wall x (m)",
                "top wall y (m)", "points");
    std::uint32_t seed = 777u;
    for (const Pose& p : poses) {
        const double yaw = p.yawDeg * std::numbers::pi / 180.0;
        const double sx = p.x + trueMountX * std::cos(yaw), sy = p.y + trueMountX * std::sin(yaw);
        const auto r = scan(sx, sy, yaw, spec, seed);
        seed += 1000u;
        double sumX = 0.0, sumY = 0.0;
        int nX = 0, nY = 0, used = 0;
        for (int i = 0; i < spec.count; ++i) {
            if (r[i] == 0.0) continue;
            const double beamDeg = spec.angleMinDeg + i * spec.angleIncDeg;
            const double a = yaw + beamDeg * std::numbers::pi / 180.0;
            // the software's belief about where the sensor is:
            const double bx = p.x + configMountX * std::cos(yaw);
            const double by = p.y + configMountX * std::sin(yaw);
            const double wx = bx + r[i] * std::cos(a), wy = by + r[i] * std::sin(a);
            ++used;
            if (wx > 2.0 && wy > -1.0 && wy < 1.5) { sumX += wx; ++nX; }
            if (wy > 1.3 && wx > -1.2 && wx < 2.2) { sumY += wy; ++nY; }
        }
        std::printf("%-44s %16.3f %14.3f %10d\n", p.what, nX ? sumX / nX : 0.0,
                    nY ? sumY / nY : 0.0, used);
    }
    return 0;
}
