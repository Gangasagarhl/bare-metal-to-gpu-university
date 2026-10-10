// no_gyro_bug.cpp - forensic evidence for F10-04. A copy of the rotational part of the
// simulator in which one term of the Newton-Euler equations was left out (see the answer key).
// Torque-free box with three different inertias, spun about an axis that is not a principal axis.
#include <cmath>
#include <cstdio>

struct V3
{
    double x, y, z;
};

int main()
{
    const double Jx = 0.010, Jy = 0.014, Jz = 0.018;  // kg m^2
    const double h = 0.001;                           // s
    V3 w{2.0, 10.0, 1.0};                             // body rates, rad/s
    // attitude as a rotation matrix R (body to world), rows r0, r1, r2; starts as identity
    double R[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    std::printf("  t(s)   wx(rad/s)  wy(rad/s)  wz(rad/s)   energy(J)    Lx_world   Ly_world"
                "   Lz_world\n");
    for (int k = 0; k <= 5000; ++k) {
        if (k % 500 == 0) {
            const double e = 0.5 * (Jx * w.x * w.x + Jy * w.y * w.y + Jz * w.z * w.z);
            const double Lb[3] = {Jx * w.x, Jy * w.y, Jz * w.z};
            double Lw[3];
            for (int i = 0; i < 3; ++i) {
                Lw[i] = R[i][0] * Lb[0] + R[i][1] * Lb[1] + R[i][2] * Lb[2];
            }
            std::printf("%6.2f %10.4f %10.4f %10.4f %11.6f %10.5f %10.5f %10.5f\n", k * h, w.x,
                        w.y, w.z, e, Lw[0], Lw[1], Lw[2]);
        }
        // rotational dynamics with zero applied torque
        const V3 dw{0.0 / Jx, 0.0 / Jy, 0.0 / Jz};
        w = {w.x + h * dw.x, w.y + h * dw.y, w.z + h * dw.z};
        // attitude kinematics dR/dt = R [w]x, integrated with the exact rotation of one step
        const double ang = std::sqrt(w.x * w.x + w.y * w.y + w.z * w.z) * h;
        const double ux = w.x * h / ang, uy = w.y * h / ang, uz = w.z * h / ang;
        const double c = std::cos(ang), s = std::sin(ang), t = 1.0 - c;
        const double D[3][3] = {{t * ux * ux + c, t * ux * uy - s * uz, t * ux * uz + s * uy},
                                {t * ux * uy + s * uz, t * uy * uy + c, t * uy * uz - s * ux},
                                {t * ux * uz - s * uy, t * uy * uz + s * ux, t * uz * uz + c}};
        double N[3][3];
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                N[i][j] = R[i][0] * D[0][j] + R[i][1] * D[1][j] + R[i][2] * D[2][j];
            }
        }
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                R[i][j] = N[i][j];
            }
        }
    }
    return 0;
}
