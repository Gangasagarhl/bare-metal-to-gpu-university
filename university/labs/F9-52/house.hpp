// house.hpp - the university's simulated house, LiDAR and random numbers (RB401).
// Used by the labs of F9-52 ... F9-58. Everything here is our own model:
// no real sensor or robot is described, and every number is a choice of this simulator.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace rb {

constexpr double kPi = 3.14159265358979323846;
constexpr double kCell = 0.10;   // metres per grid cell (our choice)
constexpr int kW = 80;           // cells in x: an 8.0 m wide house
constexpr int kH = 60;           // cells in y: a 6.0 m deep house

struct Pose
{
    double x = 0.0;      // metres
    double y = 0.0;      // metres
    double th = 0.0;     // heading, radians, 0 = +x, counter-clockwise positive
};

inline double wrapAngle(double a)
{
    while (a > kPi) { a -= 2.0 * kPi; }
    while (a <= -kPi) { a += 2.0 * kPi; }
    return a;
}

// Deterministic random numbers: the same sequence on every machine and compiler,
// because we do not use the library's distributions (their algorithms are not fixed).
class Rng
{
public:
    explicit Rng(std::uint64_t seed) : s_(seed * 2654435761u + 1u) {}
    std::uint64_t next()   // splitmix64
    {
        std::uint64_t z = (s_ += 0x9E3779B97F4A7C15ull);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
        return z ^ (z >> 31);
    }
    double uniform()       // in [0, 1)
    {
        return static_cast<double>(next() >> 11) * (1.0 / 9007199254740992.0);
    }
    double gauss(double sigma)   // Box-Muller
    {
        double u1 = uniform();
        if (u1 < 1e-300) { u1 = 1e-300; }
        const double u2 = uniform();
        return sigma * std::sqrt(-2.0 * std::log(u1)) * std::cos(2.0 * kPi * u2);
    }
private:
    std::uint64_t s_;
};

struct Rect { double x0, y0, x1, y1; };   // metres, x0 < x1, y0 < y1

// The floor plan: four rooms, three doors 0.9 m wide, four pieces of furniture.
inline std::vector<Rect> houseRects()
{
    return {
        {0.0, 0.0, 8.0, 0.1}, {0.0, 5.9, 8.0, 6.0},          // outer walls, bottom and top
        {0.0, 0.0, 0.1, 6.0}, {7.9, 0.0, 8.0, 6.0},          // outer walls, left and right
        {3.95, 0.0, 4.05, 1.2}, {3.95, 2.1, 4.05, 6.0},      // wall x = 4 m, door y 1.2-2.1
        {0.0, 3.45, 1.5, 3.55}, {2.4, 3.45, 5.5, 3.55},      // wall y = 3.5 m, door x 1.5-2.4
        {6.4, 3.45, 8.0, 3.55},                              //                 door x 5.5-6.4
        {0.5, 0.3, 2.5, 0.9},    // sofa (living room)
        {6.0, 1.0, 7.0, 2.0},    // table (kitchen)
        {0.3, 4.5, 2.0, 5.7},    // bed (bedroom)
        {7.2, 4.0, 7.8, 5.5},    // cabinet (hall)
    };
}

class Grid
{
public:
    Grid() : occ_(kW * kH, 0) {}
    bool inside(int i, int j) const { return i >= 0 && j >= 0 && i < kW && j < kH; }
    bool occ(int i, int j) const { return !inside(i, j) || occ_[j * kW + i] != 0; }
    void set(int i, int j, bool v) { if (inside(i, j)) { occ_[j * kW + i] = v ? 1 : 0; } }
    bool occAt(double x, double y) const
    {
        return occ(static_cast<int>(std::floor(x / kCell)), static_cast<int>(std::floor(y / kCell)));
    }
private:
    std::vector<std::uint8_t> occ_;
};

// A cell is occupied when its centre lies inside a rectangle of the floor plan.
inline Grid makeHouse(const std::vector<Rect>& extra = {})
{
    Grid g;
    std::vector<Rect> rs = houseRects();
    rs.insert(rs.end(), extra.begin(), extra.end());
    for (int j = 0; j < kH; ++j) {
        for (int i = 0; i < kW; ++i) {
            const double cx = (i + 0.5) * kCell;
            const double cy = (j + 0.5) * kCell;
            for (const Rect& r : rs) {
                if (cx >= r.x0 && cx <= r.x1 && cy >= r.y0 && cy <= r.y1) { g.set(i, j, true); }
            }
        }
    }
    return g;
}

// Simulated 2D LiDAR: beams evenly spaced over a full turn, ranges marched in 1 cm steps.
// A beam that meets nothing within maxRange reports exactly maxRange ("no return").
struct LidarSpec
{
    int beams = 360;
    double maxRange = 4.0;     // metres (our simulator's choice)
    double sigma = 0.02;       // metres, Gaussian range noise (our simulator's choice)
};

inline double castRay(const Grid& g, double x, double y, double ang, double maxRange)
{
    const double dx = std::cos(ang);
    const double dy = std::sin(ang);
    for (double r = 0.0; r < maxRange; r += 0.01) {
        if (g.occAt(x + r * dx, y + r * dy)) { return r; }
    }
    return maxRange;
}

inline std::vector<double> scan(const Grid& g, const Pose& p, const LidarSpec& spec, Rng& rng)
{
    std::vector<double> z(spec.beams);
    for (int k = 0; k < spec.beams; ++k) {
        const double ang = p.th + 2.0 * kPi * k / spec.beams;
        const double r = castRay(g, p.x, p.y, ang, spec.maxRange);
        z[k] = (r >= spec.maxRange) ? spec.maxRange : std::fmax(0.0, r + rng.gauss(spec.sigma));
    }
    return z;
}

// The route through all four rooms used by F9-52 and F9-53 (waypoints in metres).
inline std::vector<Pose> routeWaypoints()
{
    return {{1.0, 1.9, 0}, {3.2, 1.65, 0}, {4.8, 1.65, 0}, {5.95, 2.6, 0}, {5.95, 4.6, 0},
            {6.8, 4.8, 0}, {4.6, 4.8, 0}, {5.95, 4.6, 0}, {5.95, 2.6, 0}, {4.8, 1.65, 0},
            {3.2, 1.65, 0}, {1.95, 2.6, 0}, {1.95, 4.2, 0}, {3.2, 5.0, 0}, {1.95, 4.2, 0},
            {1.95, 2.6, 0}, {1.0, 1.9, 0}};
}

// Densify the waypoints into poses `step` metres apart, heading along the motion;
// at each corner the robot turns on the spot in steps of at most 0.15 rad.
inline std::vector<Pose> densify(const std::vector<Pose>& wp, double step)
{
    std::vector<Pose> out;
    double th = std::atan2(wp[1].y - wp[0].y, wp[1].x - wp[0].x);
    for (std::size_t k = 0; k + 1 < wp.size(); ++k) {
        const double dx = wp[k + 1].x - wp[k].x;
        const double dy = wp[k + 1].y - wp[k].y;
        const double target = std::atan2(dy, dx);
        double turn = wrapAngle(target - th);
        const int nTurn = static_cast<int>(std::ceil(std::fabs(turn) / 0.15));
        for (int t = 1; t <= nTurn; ++t) {
            out.push_back({wp[k].x, wp[k].y, wrapAngle(th + turn * t / nTurn)});
        }
        th = target;
        const double len = std::hypot(dx, dy);
        const int n = std::max(1, static_cast<int>(std::round(len / step)));
        for (int s = 0; s < n; ++s) {
            out.push_back({wp[k].x + dx * s / n, wp[k].y + dy * s / n, th});
        }
    }
    out.push_back({wp.back().x, wp.back().y, th});
    return out;
}

}  // namespace rb
