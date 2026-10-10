// quadsim.hpp - the university's small multirotor simulator (DN201, F10-04).
// World frame: x forward, y left, z up. Body frame: x forward, y left, z up (MA201 F0-53).
// State: position p and velocity v (world), attitude q (unit quaternion, body to world),
// angular velocity w (body), and the four rotor speeds (rad/s).
// All parameters are exercise values for "the course quad", not those of a real vehicle.
#pragma once
#include <algorithm>
#include <array>
#include <cmath>

namespace dn {

struct Vec3
{
    double x = 0.0, y = 0.0, z = 0.0;
};
inline Vec3 operator+(Vec3 a, Vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline Vec3 operator-(Vec3 a, Vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline Vec3 operator*(double s, Vec3 a) { return {s * a.x, s * a.y, s * a.z}; }
inline double dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Vec3 cross(Vec3 a, Vec3 b)
{
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

struct Quat
{
    double w = 1.0, x = 0.0, y = 0.0, z = 0.0;
};
inline Quat mul(Quat a, Quat b) // Hamilton product, as in MA201 F0-55
{
    return {a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z,
            a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
            a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
            a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w};
}
inline Quat normalized(Quat q)
{
    const double n = std::sqrt(q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z);
    return {q.w / n, q.x / n, q.y / n, q.z / n};
}
inline Vec3 rotate(Quat q, Vec3 v) // q v q*: a body-frame vector expressed in the world frame
{
    const Quat r = mul(mul(q, Quat{0.0, v.x, v.y, v.z}), Quat{q.w, -q.x, -q.y, -q.z});
    return {r.x, r.y, r.z};
}

struct Euler // R = Rz(yaw) Ry(pitch) Rx(roll), angles in radians (F10-03)
{
    double roll = 0.0, pitch = 0.0, yaw = 0.0;
};
inline Quat fromEuler(Euler e)
{
    const double cr = std::cos(e.roll / 2), sr = std::sin(e.roll / 2);
    const double cp = std::cos(e.pitch / 2), sp = std::sin(e.pitch / 2);
    const double cy = std::cos(e.yaw / 2), sy = std::sin(e.yaw / 2);
    return {cy * cp * cr + sy * sp * sr, cy * cp * sr - sy * sp * cr,
            cy * sp * cr + sy * cp * sr, sy * cp * cr - cy * sp * sr};
}
inline Euler toEuler(Quat q)
{
    const double s = std::clamp(2.0 * (q.w * q.y - q.z * q.x), -1.0, 1.0);
    return {std::atan2(2.0 * (q.w * q.x + q.y * q.z), 1.0 - 2.0 * (q.x * q.x + q.y * q.y)),
            std::asin(s),
            std::atan2(2.0 * (q.w * q.z + q.x * q.y), 1.0 - 2.0 * (q.y * q.y + q.z * q.z))};
}

using Rotors = std::array<double, 4>;

struct Params
{
    double mass = 1.0;                  // kg
    double g = 9.81;                    // m/s^2 (value used throughout DN201)
    double arm = 0.15;                  // m: |x| and |y| of every motor from the centre
    Vec3 inertia{0.010, 0.010, 0.018};  // kg m^2 about the body axes (principal axes)
    double kT = 1.0e-5;                 // thrust per rotor speed squared, N/(rad/s)^2
    double kQ = 1.5e-7;                 // drag torque per rotor speed squared, N m/(rad/s)^2
    double motorTau = 0.03;             // s: first-order lag of each motor (F10-05)
    double maxSpeed = 1000.0;           // rad/s
    double drag = 0.05;                 // N per (m/s)^2: quadratic body drag
    Rotors kTscale{1.0, 1.0, 1.0, 1.0}; // per-propeller factors (forensic labs change them)
    Rotors kQscale{1.0, 1.0, 1.0, 1.0};
};

// Course numbering, seen from above: 1 front-left, 2 front-right, 3 rear-right, 4 rear-left.
// Motors 1 and 3 spin clockwise and motors 2 and 4 counter-clockwise, seen from above.
constexpr Rotors motorX{+1.0, +1.0, -1.0, -1.0};  // sign of each motor's x position
constexpr Rotors motorY{+1.0, -1.0, -1.0, +1.0};  // sign of each motor's y position
constexpr Rotors spinYaw{+1.0, -1.0, +1.0, -1.0}; // sign of each rotor's reaction torque on z

struct Wrench
{
    double thrust = 0.0; // N along body z
    Vec3 torque;         // N m in the body frame
};

inline Wrench wrenchFromRotors(const Params& P, const Rotors& rotor)
{
    Wrench out;
    for (int i = 0; i < 4; ++i) {
        const double w2 = rotor[i] * rotor[i];
        const double f = P.kT * P.kTscale[i] * w2;
        out.thrust += f;
        out.torque.x += motorY[i] * P.arm * f;  // r x f with f along +z: (y f, -x f, 0)
        out.torque.y += -motorX[i] * P.arm * f;
        out.torque.z += spinYaw[i] * P.kQ * P.kQscale[i] * w2;
    }
    return out;
}

// Inverse mixer (F10-02): wanted thrust and torques -> rotor speed commands.
// It uses the nominal propeller (kT, kQ); it does not know about kTscale or kQscale.
inline Rotors mix(const Params& P, double thrust, Vec3 torque)
{
    const double c = P.kQ / P.kT;
    Rotors cmd{};
    for (int i = 0; i < 4; ++i) {
        const double f = (thrust + torque.x * motorY[i] / P.arm - torque.y * motorX[i] / P.arm
                          + torque.z * spinYaw[i] / c) / 4.0;
        cmd[i] = std::clamp(std::sqrt(std::max(f, 0.0) / P.kT), 0.0, P.maxSpeed);
    }
    return cmd;
}

struct State
{
    Vec3 p, v;
    Quat q;
    Vec3 w;
    Rotors rotor{};
};

struct Deriv
{
    Vec3 dp, dv;
    Quat dq;
    Vec3 dw;
    Rotors drotor{};
};

// Newton-Euler equations of the rigid body (F10-04) plus the motor lag (F10-05).
inline Deriv derivative(const Params& P, const State& s, const Rotors& cmd)
{
    const Wrench W = wrenchFromRotors(P, s.rotor);
    const Vec3 thrustWorld = rotate(s.q, Vec3{0.0, 0.0, W.thrust});
    const double speed = std::sqrt(dot(s.v, s.v));
    const Vec3 dragForce = (-P.drag * speed) * s.v;
    const Vec3 J = P.inertia;
    const Vec3 Jw{J.x * s.w.x, J.y * s.w.y, J.z * s.w.z};
    const Vec3 gyro = cross(s.w, Jw);
    Deriv d;
    d.dp = s.v;
    d.dv = (1.0 / P.mass) * (thrustWorld + dragForce) - Vec3{0.0, 0.0, P.g};
    d.dw = {(W.torque.x - gyro.x) / J.x, (W.torque.y - gyro.y) / J.y,
            (W.torque.z - gyro.z) / J.z};
    const Quat qd = mul(s.q, Quat{0.0, s.w.x, s.w.y, s.w.z});
    d.dq = {0.5 * qd.w, 0.5 * qd.x, 0.5 * qd.y, 0.5 * qd.z};
    for (int i = 0; i < 4; ++i) {
        const double target = std::clamp(cmd[i], 0.0, P.maxSpeed);
        d.drotor[i] = (target - s.rotor[i]) / P.motorTau;
    }
    return d;
}

inline State addScaled(const State& s, const Deriv& d, double h)
{
    State r = s;
    r.p = s.p + h * d.dp;
    r.v = s.v + h * d.dv;
    r.q = {s.q.w + h * d.dq.w, s.q.x + h * d.dq.x, s.q.y + h * d.dq.y, s.q.z + h * d.dq.z};
    r.w = s.w + h * d.dw;
    for (int i = 0; i < 4; ++i) {
        r.rotor[i] = s.rotor[i] + h * d.drotor[i];
    }
    return r;
}

// One classical Runge-Kutta (RK4) step of length h; the command is held during the step.
inline void rk4Step(const Params& P, State& s, const Rotors& cmd, double h)
{
    const Deriv k1 = derivative(P, s, cmd);
    const Deriv k2 = derivative(P, addScaled(s, k1, h / 2), cmd);
    const Deriv k3 = derivative(P, addScaled(s, k2, h / 2), cmd);
    const Deriv k4 = derivative(P, addScaled(s, k3, h), cmd);
    State n = s;
    n = addScaled(n, k1, h / 6);
    n = addScaled(n, k2, h / 3);
    n = addScaled(n, k3, h / 3);
    n = addScaled(n, k4, h / 6);
    n.q = normalized(n.q);
    s = n;
}

inline double hoverRotorSpeed(const Params& P) // all four rotors equal, thrust = m g
{
    return std::sqrt(P.mass * P.g / (4.0 * P.kT));
}

} // namespace dn
