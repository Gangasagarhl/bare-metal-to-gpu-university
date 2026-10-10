// F9-11: the university's 2D LiDAR simulator (shared by record_scan.cpp and
// mount_forensic.cpp). A room of straight walls, one box, a doorway; beams are cast
// from the sensor and the nearest wall hit is returned. Geometry, beam count, range
// limits and noise are PRETEND exercise values, not a real sensor's specification.
#pragma once
#include <cmath>
#include <cstdint>
#include <numbers>
#include <vector>

struct Seg
{
    double x1, y1, x2, y2;
};

inline std::vector<Seg> room()
{
    return {
        {-1.5, -1.2, 2.5, -1.2},                          // bottom wall
        {2.5, -1.2, 2.5, 1.8},                            // right wall
        {2.5, 1.8, 1.0, 1.8}, {0.2, 1.8, -1.5, 1.8},      // top wall with a doorway 0.2..1.0
        {-1.5, 1.8, -1.5, -1.2},                          // left wall
        {1.0, -0.6, 1.4, -0.6}, {1.4, -0.6, 1.4, -0.2},   // a box
        {1.4, -0.2, 1.0, -0.2}, {1.0, -0.2, 1.0, -0.6},
    };
}

struct ScanSpec
{
    int count = 360;
    double angleMinDeg = -180.0;
    double angleIncDeg = 1.0;
    double rangeMin = 0.15;
    double rangeMax = 6.0;
};

// Distance along the ray (ox, oy, angle a) to segment s, or a negative number if missed.
inline double hit(double ox, double oy, double a, const Seg& s)
{
    const double dx = std::cos(a), dy = std::sin(a);
    const double ex = s.x2 - s.x1, ey = s.y2 - s.y1;
    const double den = dx * ey - dy * ex;
    if (std::fabs(den) < 1e-12) return -1.0;
    const double t = ((s.x1 - ox) * ey - (s.y1 - oy) * ex) / den;
    const double u = ((s.x1 - ox) * dy - (s.y1 - oy) * dx) / den;
    if (t <= 0.0 || u < 0.0 || u > 1.0) return -1.0;
    return t;
}

// One scan from a sensor at world position (sx, sy) facing yaw (rad).
// Returns ranges in metres, rounded to millimetres; 0 means "no return".
inline std::vector<double> scan(double sx, double sy, double yaw, const ScanSpec& spec,
                                std::uint32_t seed)
{
    std::vector<double> r;
    const auto walls = room();
    std::uint32_t s = seed;
    for (int i = 0; i < spec.count; ++i) {
        const double a = yaw + (spec.angleMinDeg + i * spec.angleIncDeg) * std::numbers::pi / 180.0;
        double best = 1e9;
        for (const Seg& w : walls) {
            const double d = hit(sx, sy, a, w);
            if (d > 0.0 && d < best) best = d;
        }
        s ^= s << 13;
        s ^= s >> 17;
        s ^= s << 5;
        const double noise = 0.005 * (static_cast<double>(s) / 2147483647.5 - 1.0);  // +-5 mm
        double m = best + noise;
        if (best > spec.rangeMax || m < spec.rangeMin) m = 0.0;
        r.push_back(std::round(m * 1000.0) / 1000.0);
    }
    return r;
}
