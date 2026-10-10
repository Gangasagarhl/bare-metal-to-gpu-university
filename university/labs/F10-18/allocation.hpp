// allocation.hpp - control allocation for the course quad with rotor limits (F10-18).
// Wanted thrust T and torques (tx, ty, tz) -> four rotor forces in [0, fmax] -> speeds.
//   naive:       solve the 4 x 4 mixer exactly, then clip each force on its own (DN201's mix);
//   prioritised: keep roll and pitch first; then either thrust before yaw ("RP > T > Y")
//                or yaw before thrust ("RP > Y > T"); give up the rest as little as possible.
// The priority orders are this course's teaching choices; each autopilot documents its own.
#pragma once
#include "../F10-04/quadsim.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace alloc {

using Forces = std::array<double, 4>;

struct Wrench
{
    double T = 0.0, tx = 0.0, ty = 0.0, tz = 0.0; // N and N m
};

inline double maxForce(const dn::Params& P)
{
    return P.kT * P.maxSpeed * P.maxSpeed;
}

// What four forces really produce (nominal propellers): the forward mixer of F10-02.
inline Wrench forward(const dn::Params& P, const Forces& f)
{
    const double c = P.kQ / P.kT;
    Wrench w;
    for (int i = 0; i < 4; ++i) {
        w.T += f[i];
        w.tx += dn::motorY[i] * P.arm * f[i];
        w.ty += -dn::motorX[i] * P.arm * f[i];
        w.tz += dn::spinYaw[i] * c * f[i];
    }
    return w;
}

inline Forces naive(const dn::Params& P, const Wrench& w)
{
    const double c = P.kQ / P.kT;
    Forces f{};
    for (int i = 0; i < 4; ++i) {
        f[i] = (w.T + dn::motorY[i] * w.tx / P.arm - dn::motorX[i] * w.ty / P.arm +
                dn::spinYaw[i] * w.tz / c) /
               4.0;
        f[i] = std::clamp(f[i], 0.0, maxForce(P));
    }
    return f;
}

enum class Priority { thrustBeforeYaw, yawBeforeThrust };

inline Forces prioritised(const dn::Params& P, const Wrench& w, Priority order)
{
    const double c = P.kQ / P.kT, top = maxForce(P);
    Forces rp{}, y{};
    for (int i = 0; i < 4; ++i) {
        rp[i] = (dn::motorY[i] * w.tx / P.arm - dn::motorX[i] * w.ty / P.arm) / 4.0;
        y[i] = dn::spinYaw[i] * w.tz / c / 4.0;
    }
    auto range = [](const Forces& v) {
        return *std::max_element(v.begin(), v.end()) - *std::min_element(v.begin(), v.end());
    };
    auto add = [](const Forces& a, const Forces& b, double s) {
        return Forces{a[0] + s * b[0], a[1] + s * b[1], a[2] + s * b[2], a[3] + s * b[3]};
    };
    const double base = w.T / 4.0;
    auto fitsAtBase = [&](const Forces& v) {
        return std::all_of(v.begin(), v.end(),
                           [&](double x) { return base + x >= -1e-12 && base + x <= top + 1e-12; });
    };
    // 1. Roll and pitch first: if their spread alone exceeds [0, fmax], scale them down.
    if (range(rp) > top) {
        rp = add(Forces{}, rp, top / range(rp));
    }
    // 2. The largest yaw fraction s in [0, 1] that still fits (bisection; s = 0 always fits
    //    the range test, and fits at the requested thrust if roll and pitch do).
    auto ok = [&](double s) {
        const Forces v = add(rp, y, s);
        return order == Priority::thrustBeforeYaw ? fitsAtBase(v) : range(v) <= top + 1e-12;
    };
    double s = 0.0;
    if (ok(1.0)) {
        s = 1.0;
    } else if (ok(0.0)) {
        double lo = 0.0, hi = 1.0;
        for (int k = 0; k < 50; ++k) {
            const double mid = 0.5 * (lo + hi);
            (ok(mid) ? lo : hi) = mid;
        }
        s = lo;
    }
    const Forces v = add(rp, y, s);
    // 3. Thrust last: shift all four forces together, as little as possible, into range.
    const double low = -*std::min_element(v.begin(), v.end());
    const double high = top - *std::max_element(v.begin(), v.end());
    const double b = std::clamp(base, low, high);
    return add(Forces{b, b, b, b}, v, 1.0);
}

inline dn::Rotors toSpeeds(const dn::Params& P, const Forces& f)
{
    dn::Rotors r{};
    for (int i = 0; i < 4; ++i) {
        r[i] = std::sqrt(std::max(f[i], 0.0) / P.kT);
    }
    return r;
}

// Signatures that plug into dn301::Cascade::allocator.
inline dn::Rotors thrustBeforeYaw(const dn::Params& P, double T, dn::Vec3 tau)
{
    return toSpeeds(P, prioritised(P, {T, tau.x, tau.y, tau.z}, Priority::thrustBeforeYaw));
}
inline dn::Rotors yawBeforeThrust(const dn::Params& P, double T, dn::Vec3 tau)
{
    return toSpeeds(P, prioritised(P, {T, tau.x, tau.y, tau.z}, Priority::yawBeforeThrust));
}

} // namespace alloc
