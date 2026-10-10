// F9-25 Listing 1: forward kinematics of a serial arm as a chain of rigid transforms.
// Convention: T_AB maps B coordinates to A coordinates; T_0n = T_01 T_12 ... T_(n-1)n.
#pragma once
#include <array>
#include <cmath>
#include <numbers>
#include <vector>

using Vec3 = std::array<double, 3>;
using Mat3 = std::array<Vec3, 3>;

inline double rad(double d)
{
    return d * std::numbers::pi / 180.0;
}

struct Transform {
    Mat3 R{{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}};
    Vec3 t{0, 0, 0};
};

inline Transform operator*(const Transform& a, const Transform& b)
{
    Transform c;
    for (int i = 0; i < 3; ++i) {
        c.t[i] = a.t[i];
        for (int j = 0; j < 3; ++j) {
            c.R[i][j] = 0;
            for (int k = 0; k < 3; ++k) c.R[i][j] += a.R[i][k] * b.R[k][j];
            c.t[i] += a.R[i][j] * b.t[j];
        }
    }
    return c;
}

// rotation by angle q about the unit axis u (Rodrigues' formula)
inline Mat3 rotAxis(const Vec3& u, double q)
{
    const double c = std::cos(q), s = std::sin(q), v = 1 - c;
    return {{{c + u[0] * u[0] * v, u[0] * u[1] * v - u[2] * s, u[0] * u[2] * v + u[1] * s},
             {u[1] * u[0] * v + u[2] * s, c + u[1] * u[1] * v, u[1] * u[2] * v - u[0] * s},
             {u[2] * u[0] * v - u[1] * s, u[2] * u[1] * v + u[0] * s, c + u[2] * u[2] * v}}};
}

// one revolute joint: a fixed offset from the parent link, then a turn about 'axis'
struct Joint {
    Vec3 offset; // where the joint sits, in the parent link's frame (m)
    Vec3 axis;   // unit rotation axis, in the joint's own frame
};

struct Arm {
    std::vector<Joint> joints;
    Vec3 tool; // tool point, in the last link's frame (m)

    // T_base_tool for joint angles q (rad)
    Transform fk(const std::vector<double>& q) const
    {
        Transform T;
        for (std::size_t i = 0; i < joints.size(); ++i) {
            T = T * Transform{{{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}}, joints[i].offset}; // fixed part
            T = T * Transform{rotAxis(joints[i].axis, q[i]), {0, 0, 0}}; // joint motion
        }
        return T * Transform{{{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}}, tool};
    }
};

// the university's simulated course arm: base yaw, shoulder pitch, elbow pitch.
// Axis (0, -1, 0) makes a positive shoulder or elbow angle lift the arm upwards.
inline Arm courseArm()
{
    return {{{{0, 0, 0}, {0, 0, 1}},      // joint 1: base yaw at the base origin
             {{0, 0, 0.10}, {0, -1, 0}},  // joint 2: shoulder, 0.10 m above the base
             {{0.30, 0, 0}, {0, -1, 0}}}, // joint 3: elbow, at the end of the 0.30 m upper arm
            {0.25 + 0.05, 0, 0}};         // forearm 0.25 m plus a 0.05 m tool
}
