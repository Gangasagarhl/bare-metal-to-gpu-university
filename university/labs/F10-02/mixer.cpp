// mixer.cpp - how four propellers steer: the forward mixer (rotor forces -> thrust and torques)
// and the inverse mixer (wanted thrust and torques -> rotor forces) of the course quad (F10-02).
// Course numbering, seen from above: 1 front-left (CW), 2 front-right (CCW),
// 3 rear-right (CW), 4 rear-left (CCW). Axes: x forward, y left, z up.
#include <array>
#include <cstdio>

using Four = std::array<double, 4>;

constexpr double arm = 0.15;            // m: |x| and |y| of every motor
constexpr double c = 1.5e-7 / 1.0e-5;   // m: kQ / kT, yaw torque per newton of thrust
constexpr Four sx{+1, +1, -1, -1};      // x sign of each motor
constexpr Four sy{+1, -1, -1, +1};      // y sign of each motor
constexpr Four sz{+1, -1, +1, -1};      // yaw-torque sign (CW rotor pushes the body +z)

struct Command
{
    double thrust, roll, pitch, yaw;    // N, N m, N m, N m
};

Command forward(const Four& f)
{
    Command out{0, 0, 0, 0};
    for (int i = 0; i < 4; ++i) {
        out.thrust += f[i];
        out.roll += arm * sy[i] * f[i];
        out.pitch += -arm * sx[i] * f[i];
        out.yaw += c * sz[i] * f[i];
    }
    return out;
}

Four inverse(const Command& u)
{
    Four f{};
    for (int i = 0; i < 4; ++i) {
        f[i] = (u.thrust + u.roll * sy[i] / arm - u.pitch * sx[i] / arm + u.yaw * sz[i] / c) / 4.0;
    }
    return f;
}

void show(const char* name, const Command& u)
{
    const Four f = inverse(u);
    const Command back = forward(f);
    std::printf("%-26s %6.3f %6.3f %6.3f %6.3f | %7.4f %7.4f %7.4f %7.4f\n", name, f[0], f[1],
                f[2], f[3], back.thrust, back.roll, back.pitch, back.yaw);
}

int main()
{
    const double hover = 1.0 * 9.81;  // N: m g for the 1.0 kg course quad
    std::printf("%-26s %6s %6s %6s %6s | %7s %7s %7s %7s\n", "wanted", "f1", "f2", "f3", "f4",
                "T", "tau_x", "tau_y", "tau_z");
    show("hover", {hover, 0.0, 0.0, 0.0});
    show("roll +0.1 N m (right down)", {hover, 0.1, 0.0, 0.0});
    show("pitch +0.1 N m (nose down)", {hover, 0.0, 0.1, 0.0});
    show("yaw +0.01 N m (nose left)", {hover, 0.0, 0.0, 0.01});
    show("climb: thrust x 1.2", {1.2 * hover, 0.0, 0.0, 0.0});
    show("yaw +0.2 N m (too much)", {hover, 0.0, 0.0, 0.2});
    return 0;
}
