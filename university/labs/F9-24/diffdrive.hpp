// F9-24 Listing 1: differential-drive kinematics (simulated course robot).
// Wheel speeds are in rad/s, r = wheel radius (m), W = track width (m, wheel to wheel).
#pragma once
#include <cmath>

struct Pose {
    double x = 0, y = 0, theta = 0; // metres, metres, radians (world frame)
};
struct Twist2 {
    double v = 0, w = 0; // forward speed (m/s) and turn rate (rad/s), body frame
};
struct Wheels {
    double left = 0, right = 0; // wheel angular speeds (rad/s)
};

// forward kinematics: wheel speeds -> body velocity
inline Twist2 forwardKin(const Wheels& q, double r, double W)
{
    return {r * (q.right + q.left) / 2.0, r * (q.right - q.left) / W};
}

// inverse kinematics: desired body velocity -> wheel speeds
inline Wheels inverseKin(const Twist2& b, double r, double W)
{
    return {(b.v - b.w * W / 2.0) / r, (b.v + b.w * W / 2.0) / r};
}

// pose update, method 1: Euler step (straight line along the old heading)
inline Pose stepEuler(Pose p, const Twist2& b, double dt)
{
    p.x += b.v * std::cos(p.theta) * dt;
    p.y += b.v * std::sin(p.theta) * dt;
    p.theta += b.w * dt;
    return p;
}

// pose update, method 2: exact arc for constant v and w during dt
inline Pose stepArc(Pose p, const Twist2& b, double dt)
{
    const double dth = b.w * dt;
    if (std::abs(dth) < 1e-12) return stepEuler(p, b, dt); // straight line
    const double R = b.v / b.w;                            // signed turning radius
    p.x += R * (std::sin(p.theta + dth) - std::sin(p.theta));
    p.y -= R * (std::cos(p.theta + dth) - std::cos(p.theta));
    p.theta += dth;
    return p;
}
