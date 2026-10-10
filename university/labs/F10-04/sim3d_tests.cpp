// sim3d_tests.cpp - tests of the 3D simulator in quadsim.hpp against results we can derive
// by hand: hover, free fall, the hover thrust condition when tilted, torque signs, the
// conservation laws of a spinning body, and step-size convergence.
#include "quadsim.hpp"

#include <cmath>
#include <cstdio>
#include <numbers>

namespace {

int failures = 0;

void check(const char* name, bool ok, double value)
{
    std::printf("%-58s %-4s (%.3e)\n", name, ok ? "PASS" : "FAIL", value);
    if (!ok) {
        ++failures;
    }
}

dn::State hoverState(const dn::Params& P)
{
    dn::State s;
    const double w = dn::hoverRotorSpeed(P);
    s.rotor = {w, w, w, w};
    return s;
}

void run(const dn::Params& P, dn::State& s, const dn::Rotors& cmd, double seconds, double h)
{
    const int steps = static_cast<int>(std::lround(seconds / h));
    for (int k = 0; k < steps; ++k) {
        dn::rk4Step(P, s, cmd, h);
    }
}

double length(dn::Vec3 a) { return std::sqrt(dn::dot(a, a)); }

} // namespace

int main()
{
    const double h = 0.001;
    dn::Params P;
    std::printf("hover rotor speed of the course quad: %.2f rad/s\n", dn::hoverRotorSpeed(P));

    { // T1: four equal rotors at hover speed, level: nothing may move.
        dn::State s = hoverState(P);
        run(P, s, s.rotor, 5.0, h);
        check("T1 hover: position after 5 s", length(s.p) < 1e-9, length(s.p));
    }
    { // T2: rotors stopped, no drag: z(t) = -g t^2 / 2.
        dn::Params Q = P;
        Q.drag = 0.0;
        dn::State s;
        run(Q, s, dn::Rotors{}, 2.0, h);
        const double err = std::fabs(s.p.z - (-0.5 * Q.g * 4.0));
        std::printf("   free fall: z(2 s) = %.6f m\n", s.p.z);
        check("T2 free fall: z(2 s) equals -g t^2/2", err < 1e-9, err);
    }
    { // T3: hover thrust condition when tilted: T = m g / (cos(roll) cos(pitch)).
        dn::Params Q = P;
        Q.drag = 0.0;
        dn::State s;
        const double roll = 20.0 * std::numbers::pi / 180.0;
        const double pitch = -10.0 * std::numbers::pi / 180.0;
        s.q = dn::fromEuler({roll, pitch, 0.0});
        const double thrust = Q.mass * Q.g / (std::cos(roll) * std::cos(pitch));
        const double w = std::sqrt(thrust / (4.0 * Q.kT));
        s.rotor = {w, w, w, w};
        run(Q, s, s.rotor, 1.0, h);
        std::printf("   tilted: thrust %.4f N, v after 1 s = (%.4f, %.4f, %.2e) m/s\n", thrust,
                    s.v.x, s.v.y, s.v.z);
        check("T3 tilted hover thrust: vertical speed stays zero", std::fabs(s.v.z) < 1e-9,
              std::fabs(s.v.z));
        const dn::Vec3 thrustDir = dn::rotate(s.q, {0.0, 0.0, 1.0});
        const double ax = thrust * thrustDir.x / Q.mass, ay = thrust * thrustDir.y / Q.mass;
        check("T3 tilted hover thrust: horizontal speed = a * 1 s",
              std::fabs(s.v.x - ax) + std::fabs(s.v.y - ay) < 1e-9,
              std::fabs(s.v.x - ax) + std::fabs(s.v.y - ay));
    }
    { // T4: left motors (1, 4) faster -> positive roll rate; CW motors (1, 3) faster -> +yaw.
        dn::State s = hoverState(P);
        dn::Rotors cmd = s.rotor;
        cmd[0] += 20.0;
        cmd[3] += 20.0;
        cmd[1] -= 20.0;
        cmd[2] -= 20.0;
        run(P, s, cmd, 0.1, h);
        check("T4 left motors faster: roll rate > 0 (right side down)", s.w.x > 0.0, s.w.x);
        dn::State y = hoverState(P);
        cmd = y.rotor;
        cmd[0] += 20.0;
        cmd[2] += 20.0;
        cmd[1] -= 20.0;
        cmd[3] -= 20.0;
        run(P, y, cmd, 0.1, h);
        check("T4 clockwise motors faster: yaw rate > 0 (nose turns left)", y.w.z > 0.0, y.w.z);
    }
    { // T5: torque-free spin about the intermediate axis of a box with three different inertias.
        dn::Params Q = P;
        Q.inertia = {0.010, 0.014, 0.018};
        Q.drag = 0.0;
        Q.g = 0.0;
        dn::State s;
        s.w = {0.001, 10.0, 0.001};  // almost pure spin about body y
        auto energy = [&](const dn::State& st) {
            return 0.5 * (Q.inertia.x * st.w.x * st.w.x + Q.inertia.y * st.w.y * st.w.y
                          + Q.inertia.z * st.w.z * st.w.z);
        };
        auto momentum = [&](const dn::State& st) {  // angular momentum in the world frame
            return dn::rotate(st.q, {Q.inertia.x * st.w.x, Q.inertia.y * st.w.y,
                                     Q.inertia.z * st.w.z});
        };
        const double e0 = energy(s);
        const dn::Vec3 L0 = momentum(s);
        double minWy = s.w.y;
        for (int k = 0; k < 5000; ++k) {
            dn::rk4Step(Q, s, dn::Rotors{}, h);
            minWy = std::min(minWy, s.w.y);
        }
        const double eErr = std::fabs(energy(s) - e0) / e0;
        const double lErr = length(momentum(s) - L0) / length(L0);
        std::printf("   spin about y: lowest body y rate in 5 s = %.3f rad/s (it flipped)\n",
                    minWy);
        check("T5 torque-free spin: kinetic energy conserved", eErr < 1e-6, eErr);
        check("T5 torque-free spin: world angular momentum conserved", lErr < 1e-6, lErr);
        check("T5 torque-free spin: intermediate axis is unstable", minWy < -9.0, minWy);
    }
    { // T6: halving the step must change a 2 s manoeuvre very little (convergence).
        dn::State a = hoverState(P), b = hoverState(P);
        dn::Rotors cmd = a.rotor;
        cmd[0] += 30.0;
        cmd[1] += 10.0;
        run(P, a, cmd, 2.0, 0.002);
        run(P, b, cmd, 2.0, 0.001);
        const double diff = length(a.p - b.p);
        check("T6 step 2 ms vs 1 ms: position difference after 2 s < 1 mm", diff < 1e-3, diff);
    }
    std::printf("%s: %d failure(s)\n", failures == 0 ? "ALL TESTS PASSED" : "TESTS FAILED",
                failures);
    return failures == 0 ? 0 : 1;
}
