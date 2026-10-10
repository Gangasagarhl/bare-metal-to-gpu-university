// yaw_climb.cpp - forensic evidence for F10-02 ("It yaws when it climbs").
// Flies the course quad of F10-04 in the university simulator with a simple controller
// (vertical speed P, roll/pitch PID, yaw-rate PI: DN301 teaches these properly) and prints
// the flight log: vertical speed, the four rotor speeds, yaw rate and heading.
// Two flights: the reference log recorded last week, and today's log. Between them one part
// of the vehicle was changed; what and how is in the answer key of F10-02.
#include "../F10-04/quadsim.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <numbers>

namespace {

void fly(const char* title, const dn::Params& P)
{
    const double h = 0.001, deg = 180.0 / std::numbers::pi;
    dn::State s;
    const double w0 = dn::hoverRotorSpeed(P);
    s.rotor = {w0, w0, w0, w0};
    dn::Rotors cmd = s.rotor;
    double rollIntegral = 0.0, pitchIntegral = 0.0, yawIntegral = 0.0;
    std::printf("%s\n", title);
    std::printf(" t(s) vz_set  vz(m/s)  w1(rad/s)  w2(rad/s)  w3(rad/s)  w4(rad/s) "
                "yawrate(deg/s) heading(deg)\n");
    for (int k = 0; k <= 8500; ++k) {
        const double t = k * h;
        const double vzSet = (k >= 4000 && k < 7000) ? 4.0 : 0.0;  // climb from 4 s to 7 s
        if (k % 2 == 0) {  // the controller runs every 2 ms
            const dn::Euler e = dn::toEuler(s.q);
            const double az = std::clamp(8.0 * (vzSet - s.v.z), -5.0, 5.0);
            const double thrust = P.mass * (P.g + az) / (std::cos(e.roll) * std::cos(e.pitch));
            const dn::Vec3 J = P.inertia;
            const double yawErr = 0.0 - s.w.z;
            rollIntegral += (0.0 - e.roll) * 0.002;
            pitchIntegral += (0.0 - e.pitch) * 0.002;
            yawIntegral += yawErr * 0.002;
            const dn::Vec3 torque{
                J.x * (64.0 * (0.0 - e.roll) - 12.8 * s.w.x + 100.0 * rollIntegral),
                J.y * (64.0 * (0.0 - e.pitch) - 12.8 * s.w.y + 100.0 * pitchIntegral),
                J.z * (5.0 * yawErr + 6.0 * yawIntegral)};
            cmd = dn::mix(P, thrust, torque);
        }
        if (k >= 3500 && k % 250 == 0) {
            std::printf("%5.2f %6.1f %8.3f %10.1f %10.1f %10.1f %10.1f %14.2f %12.2f\n", t, vzSet,
                        s.v.z, s.rotor[0], s.rotor[1], s.rotor[2], s.rotor[3], s.w.z * deg,
                        dn::toEuler(s.q).yaw * deg);
        }
        dn::rk4Step(P, s, cmd, h);
    }
}

} // namespace

int main()
{
    dn::Params reference;
    reference.drag = 0.2;  // the vehicle with its camera and landing gear
    fly("=== reference flight (last week) ===", reference);
    dn::Params today = reference;
    today.kTscale[2] = 1.2;  // the change made between the flights (answer key)
    today.kQscale[2] = 1.6;
    fly("=== today's flight ===", today);
    return 0;
}
