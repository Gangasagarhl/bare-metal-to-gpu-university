// F9-25 Listing 3: the same arm with the product-of-exponentials formula,
// T(q) = exp([S1] q1) exp([S2] q2) exp([S3] q3) M, compared with the chain of Listing 1.
#include "arm.hpp"
#include <algorithm>
#include <cstdio>

Vec3 cross(const Vec3& a, const Vec3& b)
{
    return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
}

// print -0 as 0
double tidy(double x)
{
    return x == 0.0 ? 0.0 : x;
}

// exponential of a revolute screw: axis w (unit) through point p, angle q
Transform screwExp(const Vec3& w, const Vec3& p, double q)
{
    Transform T;
    T.R = rotAxis(w, q);
    // a rotation about an axis through p: x -> R (x - p) + p, so t = p - R p
    for (int i = 0; i < 3; ++i) {
        T.t[i] = p[i];
        for (int j = 0; j < 3; ++j) T.t[i] -= T.R[i][j] * p[j];
    }
    return T;
}

int main()
{
    // home configuration (all joints zero): tool at (0.6, 0, 0.1), not rotated
    const Transform M{{{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}}, {0.60, 0, 0.10}};
    // screw axes in the base frame at the home configuration: direction and a point on each axis
    const Vec3 w1{0, 0, 1}, p1{0, 0, 0};
    const Vec3 w2{0, -1, 0}, p2{0, 0, 0.10};
    const Vec3 w3{0, -1, 0}, p3{0.30, 0, 0.10};
    const Vec3 v2 = cross(p2, w2), v3 = cross(p3, w3); // linear part v = -w x p = p x w
    std::printf("screw S2: w = (0, -1, 0), v = (%.2f, %.2f, %.2f)\n", tidy(v2[0]), tidy(v2[1]),
                tidy(v2[2]));
    std::printf("screw S3: w = (0, -1, 0), v = (%.2f, %.2f, %.2f)\n", tidy(v3[0]), tidy(v3[1]),
                tidy(v3[2]));

    const Arm arm = courseArm();
    double worst = 0;
    int count = 0;
    for (int a = -180; a <= 180; a += 30)
        for (int b = -90; b <= 90; b += 15)
            for (int c = -150; c <= 150; c += 25) {
                const double q1 = rad(a), q2 = rad(b), q3 = rad(c);
                const Transform P =
                    screwExp(w1, p1, q1) * screwExp(w2, p2, q2) * screwExp(w3, p3, q3) * M;
                const Transform C = arm.fk({q1, q2, q3});
                for (int i = 0; i < 3; ++i) {
                    worst = std::max(worst, std::abs(P.t[i] - C.t[i]));
                    for (int j = 0; j < 3; ++j)
                        worst = std::max(worst, std::abs(P.R[i][j] - C.R[i][j]));
                }
                ++count;
            }
    std::printf("checked %d configurations; largest difference PoE vs chain (R and t): %.3e\n",
                count, worst);
    return worst < 1e-12 ? 0 : 1;
}
