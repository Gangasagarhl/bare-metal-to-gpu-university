// slam.hpp - F9-53: noisy odometry and a correlative scan-to-map matcher.
// Builds on ../F9-52/house.hpp (world, LiDAR) and ../F9-52/occgrid.hpp (map).
#pragma once
#include "../F9-52/occgrid.hpp"
#include <vector>

namespace rb {

// Compose a pose with a motion given in the robot's own frame (forward, left, turn).
inline Pose compose(const Pose& p, double fwd, double left, double dth)
{
    const double c = std::cos(p.th);
    const double s = std::sin(p.th);
    return {p.x + c * fwd - s * left, p.y + s * fwd + c * left, wrapAngle(p.th + dth)};
}

// The motion from pose a to pose b, expressed in a's frame.
struct Delta { double fwd, left, dth; };
inline Delta between(const Pose& a, const Pose& b)
{
    const double c = std::cos(a.th);
    const double s = std::sin(a.th);
    const double dx = b.x - a.x;
    const double dy = b.y - a.y;
    return {c * dx + s * dy, -s * dx + c * dy, wrapAngle(b.th - a.th)};
}

// Wheel odometry model of this simulator: a 2 % scale error on distance (for example a
// wrong wheel diameter), a turn bias of 0.02 rad per metre driven (unequal wheels),
// plus small random noise on every step. Chosen values, not a real robot's.
struct OdoNoise
{
    double scale = 1.02;
    double turnPerMetre = 0.02;
    double sigmaFwd = 0.002;
    double sigmaTh = 0.002;
};

inline Delta noisy(const Delta& d, const OdoNoise& n, Rng& rng)
{
    const double dist = std::hypot(d.fwd, d.left);
    return {d.fwd * n.scale + rng.gauss(n.sigmaFwd) * (dist > 0 ? 1.0 : 0.0), d.left * n.scale,
            d.dth + n.turnPerMetre * dist + rng.gauss(n.sigmaTh)};
}

// Correlative scan-to-map matching: try every pose in a small window around the
// prediction and keep the one whose beam end points land on the most occupied cells.
// Score of a pose = sum over beams with a return of max(0, log-odds of the end cell).
struct MatchResult { Pose pose; double score; };

inline double matchScore(const OccGrid& m, const Pose& p, const std::vector<double>& z,
                         double maxRange, int beamStep)
{
    const int n = static_cast<int>(z.size());
    double sc = 0.0;
    for (int k = 0; k < n; k += beamStep) {
        if (z[k] >= maxRange) { continue; }
        const double a = p.th + 2.0 * kPi * k / n;
        const int i = static_cast<int>(std::floor((p.x + z[k] * std::cos(a)) / kCell));
        const int j = static_cast<int>(std::floor((p.y + z[k] * std::sin(a)) / kCell));
        if (i < 0 || j < 0 || i >= kW || j >= kH) { continue; }
        sc += std::fmax(0.0, m.logOdds(i, j));
    }
    return sc;
}

inline MatchResult match(const OccGrid& m, const Pose& guess, const std::vector<double>& z,
                         double maxRange)
{
    MatchResult best{guess, -1.0};
    // coarse search: +-0.15 m in 0.05 m steps, +-0.08 rad in 0.02 rad steps, every 2nd beam
    for (int a = -4; a <= 4; ++a) {
        for (int ix = -3; ix <= 3; ++ix) {
            for (int iy = -3; iy <= 3; ++iy) {
                const Pose p{guess.x + 0.05 * ix, guess.y + 0.05 * iy, wrapAngle(guess.th + 0.02 * a)};
                const double s = matchScore(m, p, z, maxRange, 2);
                if (s > best.score) { best = {p, s}; }
            }
        }
    }
    // fine search around the coarse winner: +-0.025 m in 0.0125 m steps, +-0.01 rad in 0.005 rad
    const Pose c = best.pose;
    best.score = -1.0;
    for (int a = -2; a <= 2; ++a) {
        for (int ix = -2; ix <= 2; ++ix) {
            for (int iy = -2; iy <= 2; ++iy) {
                const Pose p{c.x + 0.0125 * ix, c.y + 0.0125 * iy, wrapAngle(c.th + 0.005 * a)};
                const double s = matchScore(m, p, z, maxRange, 1);
                if (s > best.score) { best = {p, s}; }
            }
        }
    }
    return best;
}

}  // namespace rb
