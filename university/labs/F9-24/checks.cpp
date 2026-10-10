// F9-24 checks: recomputes the numbers of the worked example and the answers.
#include "diffdrive.hpp"
#include <cstdio>
#include <numbers>

int main()
{
    const double r = 0.05, W = 0.30;
    const Wheels a = inverseKin({0.4, 0.5}, r, W);
    std::printf("IK v=0.4, w=0.5      -> left %.4f, right %.4f rad/s\n", a.left, a.right);
    const Twist2 b = forwardKin(a, r, W);
    std::printf("FK back              -> v %.4f m/s, w %.4f rad/s\n", b.v, b.w);
    std::printf("turning radius v/w   -> %.4f m\n", b.v / b.w);
    const Wheels s = inverseKin({0.0, 1.0}, r, W);
    std::printf("spin w=1             -> left %.4f, right %.4f rad/s, 90 deg takes %.4f s\n",
                s.left, s.right, (std::numbers::pi / 2) / 1.0);
    const Pose e = stepEuler({}, b, 0.5), c = stepArc({}, b, 0.5);
    std::printf("one 0.5 s step Euler -> (%.5f, %.5f, %.4f)\n", e.x, e.y, e.theta);
    std::printf("one 0.5 s step arc   -> (%.5f, %.5f, %.4f)\n", c.x, c.y, c.theta);
    const Twist2 q = forwardKin({6.0, 10.0}, r, W);
    std::printf("left 6, right 10     -> v %.4f, w %.4f, radius %.4f m\n", q.v, q.w, q.v / q.w);
    const Twist2 u = forwardKin({10.0, 6.0}, r, W);
    std::printf("left 10, right 6     -> v %.4f, w %.4f\n", u.v, u.w);
    std::printf("wheel too wide: 90 deg x 0.30/0.34 = %.2f deg\n", 90.0 * 0.30 / 0.34);
    std::printf("left wheel 1 percent small, 10 m straight: heading error %.4f rad\n",
                (0.01 * 10.0) / W);
    return 0;
}
