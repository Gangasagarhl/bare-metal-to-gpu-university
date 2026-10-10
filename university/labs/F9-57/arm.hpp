// arm.hpp - F9-57: a pick-and-place pipeline for a three-joint planar arm, written to show
// the STEPS a motion-planning framework such as MoveIt 2 performs (planning scene, IK,
// joint-space planning, Cartesian moves, attached objects, allowed contacts, time
// parameterisation). It is our own code, not MoveIt and not its API.
#pragma once
#include "../F9-52/house.hpp"
#include <algorithm>
#include <array>
#include <cstdio>
#include <optional>
#include <string>
#include <vector>

namespace arm {

using Q = std::array<double, 3>;          // joint angles (rad)
constexpr double kPi = rb::kPi;
constexpr double kL1 = 0.50, kL2 = 0.40, kL3 = 0.15;   // link lengths (m), our model
constexpr double kLinkRadius = 0.03;                    // links are capsules of this radius
constexpr Q kQMin = {-0.2, -2.6, -4.8};   // joint limits (rad), our model
constexpr Q kQMax = {3.3, 2.6, 1.0};
constexpr double kVMax = 1.0;             // rad/s, every joint (our model)
constexpr double kAMax = 2.0;             // rad/s^2, every joint (our model)

struct P2 { double x, z; };
struct Box { double x0, z0, x1, z1; const char* name; };

// The scene: the arm's first joint sits on a pedestal 0.40 m above a table.
// Coordinates: x horizontal, z up (m), table surface at z = 0.
constexpr double kBaseZ = 0.40;
inline std::vector<Box> sceneBoxes()
{
    return {{-1.0, -0.05, 1.2, 0.0, "table"},
            {-0.08, 0.0, 0.08, kBaseZ, "pedestal"},
            {0.18, 0.0, 0.36, 0.15, "shelf"},
            {0.40, 0.0, 0.44, 0.30, "lamp"}};
}
constexpr double kObjW = 0.06, kObjH = 0.08;            // the object to move (a small box)

// Forward kinematics: base, elbow, wrist, tool tip.
inline std::array<P2, 4> fk(const Q& q)
{
    const double a1 = q[0], a2 = q[0] + q[1], a3 = q[0] + q[1] + q[2];
    const P2 e{kL1 * std::cos(a1), kBaseZ + kL1 * std::sin(a1)};
    const P2 w{e.x + kL2 * std::cos(a2), e.z + kL2 * std::sin(a2)};
    return {P2{0, kBaseZ}, e, w, P2{w.x + kL3 * std::cos(a3), w.z + kL3 * std::sin(a3)}};
}

// Inverse kinematics: tool tip at (x, z) with tool angle phi; elbow = +1 or -1.
inline std::optional<Q> ik(double x, double z, double phi, int elbow)
{
    const double wx = x - kL3 * std::cos(phi);
    const double wz = z - kBaseZ - kL3 * std::sin(phi);     // wrist, relative to the first joint
    const double c2 = (wx * wx + wz * wz - kL1 * kL1 - kL2 * kL2) / (2.0 * kL1 * kL2);
    if (c2 < -1.0 || c2 > 1.0) { return std::nullopt; }      // out of reach
    const double q2 = elbow * std::acos(c2);
    const double q1 = std::atan2(wz, wx) - std::atan2(kL2 * std::sin(q2), kL1 + kL2 * std::cos(q2));
    Q q{q1, q2, rb::wrapAngle(phi - q1 - q2)};
    if (q[2] > kQMax[2]) { q[2] -= 2.0 * kPi; }            // the same tool angle, inside the limits
    if (q[2] < kQMin[2]) { q[2] += 2.0 * kPi; }
    for (int j = 0; j < 3; ++j) {
        if (q[j] < kQMin[j] || q[j] > kQMax[j]) { return std::nullopt; }   // outside joint limits
    }
    return q;
}

inline double distToBox(P2 p, const Box& b)
{
    const double dx = std::fmax(std::fmax(b.x0 - p.x, 0.0), p.x - b.x1);
    const double dz = std::fmax(std::fmax(b.z0 - p.z, 0.0), p.z - b.z1);
    return std::hypot(dx, dz);
}

// The planning scene: fixed boxes, the object (resting, or held by the tool), allowed contacts.
struct Scene
{
    std::vector<Box> fixed = sceneBoxes();
    P2 objectAt{0.62, 0.0};          // bottom centre of the object while it rests somewhere
    bool held = false;               // attached: the object moves rigidly with the tool
    bool objectInModel = true;       // false = the planner does not know about the object at all
    bool allowToolObject = false;    // allowed collision: tool (link 3) with the object

    // A held object is a capsule of radius kObjW/2 along the tool axis below the tip, whose
    // far end lies kObjH from the tip; these are points on its centre line.
    std::vector<P2> heldAxis(const Q& q) const
    {
        const P2 t = fk(q)[3];
        const double phi = q[0] + q[1] + q[2];
        std::vector<P2> pts;
        for (int k = 0; k <= 8; ++k) {
            const double s = (kObjH - kObjW / 2) * k / 8.0;
            pts.push_back({t.x + s * std::cos(phi), t.z + s * std::sin(phi)});
        }
        return pts;
    }
    Box restingBox() const
    {
        return {objectAt.x - kObjW / 2, objectAt.z, objectAt.x + kObjW / 2, objectAt.z + kObjH, "object"};
    }

    // The name of the first collision found ("" if none). Links 1-2 against everything;
    // link 3 (the tool) against everything except the object when contact is allowed or it
    // is held; a held object against the fixed boxes.
    std::string collision(const Q& q) const
    {
        for (int j = 0; j < 3; ++j) {
            if (q[j] < kQMin[j] || q[j] > kQMax[j]) { return "joint limit"; }
        }
        const std::array<P2, 4> f = fk(q);
        const std::vector<P2> axis = held ? heldAxis(q) : std::vector<P2>{};
        const Box rest = restingBox();
        for (int link = 0; link < 3; ++link) {
            for (int k = 0; k <= 20; ++k) {
                const double s = k / 20.0;
                const P2 p{f[link].x + s * (f[link + 1].x - f[link].x), f[link].z + s * (f[link + 1].z - f[link].z)};
                for (const Box& b : fixed) {
                    if (link == 0 && b.name[0] == 'p') { continue; }   // link 1 starts on the pedestal
                    if (distToBox(p, b) < kLinkRadius) { return std::string("link ") + char('1' + link) + " - " + b.name; }
                }
                if (!objectInModel || (link == 2 && (allowToolObject || held))) { continue; }
                if (!held && distToBox(p, rest) < kLinkRadius) {
                    return std::string("link ") + char('1' + link) + " - object";
                }
                for (const P2& a : axis) {
                    if (std::hypot(p.x - a.x, p.z - a.z) < kLinkRadius + kObjW / 2) {
                        return std::string("link ") + char('1' + link) + " - held object";
                    }
                }
            }
        }
        if (held && objectInModel) {
            for (const P2& a : axis) {
                for (const Box& b : fixed) {
                    if (distToBox(a, b) < kObjW / 2 - 1e-4) { return std::string("held object - ") + b.name; }
                }
            }
        }
        return "";
    }
    bool edgeFree(const Q& a, const Q& b, double res = 0.02) const
    {
        double d = 0.0;
        for (int j = 0; j < 3; ++j) { d = std::fmax(d, std::fabs(b[j] - a[j])); }
        const int n = std::max(1, static_cast<int>(std::ceil(d / res)));
        for (int k = 0; k <= n; ++k) {
            Q q;
            for (int j = 0; j < 3; ++j) { q[j] = a[j] + (b[j] - a[j]) * k / n; }
            if (!collision(q).empty()) { return false; }
        }
        return true;
    }
};

inline double qdist(const Q& a, const Q& b)
{
    return std::sqrt((a[0] - b[0]) * (a[0] - b[0]) + (a[1] - b[1]) * (a[1] - b[1]) + (a[2] - b[2]) * (a[2] - b[2]));
}

// RRT-Connect in joint space: grow a tree from each end, try to join them after every step.
inline std::vector<Q> rrtConnect(const Scene& sc, const Q& start, const Q& goal, rb::Rng& rng, int maxIter = 4000)
{
    struct T { std::vector<Q> q; std::vector<int> parent; };
    T ta{{start}, {-1}};
    T tb{{goal}, {-1}};
    const double step = 0.15;
    auto nearest = [](const T& t, const Q& q) {
        int best = 0;
        for (std::size_t k = 1; k < t.q.size(); ++k) {
            if (qdist(t.q[k], q) < qdist(t.q[best], q)) { best = static_cast<int>(k); }
        }
        return best;
    };
    auto extend = [&](T& t, const Q& target) -> int {        // one step towards target; -1 if blocked
        const int n = nearest(t, target);
        const double d = qdist(t.q[n], target);
        Q q = target;
        if (d > step) { for (int j = 0; j < 3; ++j) { q[j] = t.q[n][j] + (target[j] - t.q[n][j]) * step / d; } }
        if (!sc.edgeFree(t.q[n], q)) { return -1; }
        t.q.push_back(q);
        t.parent.push_back(n);
        return static_cast<int>(t.q.size()) - 1;
    };
    T* a = &ta;
    T* b = &tb;
    for (int it = 0; it < maxIter; ++it) {
        Q r;
        for (int j = 0; j < 3; ++j) { r[j] = kQMin[j] + rng.uniform() * (kQMax[j] - kQMin[j]); }
        const int na = extend(*a, r);
        if (na >= 0) {
            int nb = extend(*b, a->q[na]);                         // connect: keep stepping towards it
            while (nb >= 0 && qdist(b->q[nb], a->q[na]) > 1e-9) { nb = extend(*b, a->q[na]); }
            if (nb >= 0) {
                std::vector<Q> pa;
                std::vector<Q> pb;
                for (int k = na; k != -1; k = a->parent[k]) { pa.insert(pa.begin(), a->q[k]); }
                for (int k = nb; k != -1; k = b->parent[k]) { pb.push_back(b->q[k]); }
                pa.insert(pa.end(), pb.begin() + 1, pb.end());
                if (a != &ta) { std::reverse(pa.begin(), pa.end()); }
                return pa;
            }
        }
        std::swap(a, b);
    }
    return {};
}

inline std::vector<Q> shortcut(const Scene& sc, std::vector<Q> p, int tries, rb::Rng& rng)
{
    for (int t = 0; t < tries && p.size() > 2; ++t) {
        const std::size_t i = static_cast<std::size_t>(rng.uniform() * static_cast<double>(p.size()));
        const std::size_t j = static_cast<std::size_t>(rng.uniform() * static_cast<double>(p.size()));
        const std::size_t lo = std::min(i, j), hi = std::max(i, j);
        if (hi > lo + 1 && sc.edgeFree(p[lo], p[hi])) {
            p.erase(p.begin() + static_cast<long>(lo) + 1, p.begin() + static_cast<long>(hi));
        }
    }
    return p;
}

// Straight-line (Cartesian) tool move by dz metres in 1 cm steps, solved by IK at each step.
inline std::vector<Q> cartesianZ(const Q& from, double dz, int elbow)
{
    std::vector<Q> out{from};
    const P2 t = fk(from)[3];
    const double phi = from[0] + from[1] + from[2];
    const int n = static_cast<int>(std::ceil(std::fabs(dz) / 0.01));
    for (int k = 1; k <= n; ++k) {
        const std::optional<Q> q = ik(t.x, t.z + dz * k / n, phi, elbow);
        if (!q) { return {}; }
        out.push_back(*q);
    }
    return out;
}

// Time parameterisation: the arm stops at every waypoint; each segment uses a trapezoidal
// (or triangular) velocity profile for the joint that moves furthest, the others scaled.
inline double segmentTime(const Q& a, const Q& b)
{
    double d = 0.0;
    for (int j = 0; j < 3; ++j) { d = std::fmax(d, std::fabs(b[j] - a[j])); }
    if (d <= kVMax * kVMax / kAMax) { return 2.0 * std::sqrt(d / kAMax); }   // never reaches kVMax
    return d / kVMax + kVMax / kAMax;
}

inline double duration(const std::vector<Q>& p)
{
    double t = 0.0;
    for (std::size_t k = 1; k < p.size(); ++k) { t += segmentTime(p[k - 1], p[k]); }
    return t;
}

}  // namespace arm
