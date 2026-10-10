// attitude.cpp - roll, pitch and yaw of a multirotor (F10-03).
// Convention of DN201: axes x forward, y left, z up (body and world);
// R = Rz(yaw) Ry(pitch) Rx(roll) turns body-frame vectors into world-frame vectors.
#include <array>
#include <cmath>
#include <cstdio>
#include <numbers>

using Mat3 = std::array<std::array<double, 3>, 3>;
constexpr double deg = std::numbers::pi / 180.0;

Mat3 multiply(const Mat3& a, const Mat3& b)
{
    Mat3 c{};
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            for (int k = 0; k < 3; ++k) {
                c[i][j] += a[i][k] * b[k][j];
            }
        }
    }
    return c;
}

Mat3 fromEuler(double roll, double pitch, double yaw)
{
    const double cr = std::cos(roll), sr = std::sin(roll);
    const double cp = std::cos(pitch), sp = std::sin(pitch);
    const double cy = std::cos(yaw), sy = std::sin(yaw);
    const Mat3 Rx{{{1, 0, 0}, {0, cr, -sr}, {0, sr, cr}}};
    const Mat3 Ry{{{cp, 0, sp}, {0, 1, 0}, {-sp, 0, cp}}};
    const Mat3 Rz{{{cy, -sy, 0}, {sy, cy, 0}, {0, 0, 1}}};
    return multiply(Rz, multiply(Ry, Rx));
}

void toEuler(const Mat3& R, double& roll, double& pitch, double& yaw)
{
    pitch = std::asin(-R[2][0]);
    roll = std::atan2(R[2][1], R[2][2]);
    yaw = std::atan2(R[1][0], R[0][0]);
}

int main()
{
    const double roll = 10 * deg, pitch = -20 * deg, yaw = 30 * deg;
    const Mat3 R = fromEuler(roll, pitch, yaw);
    std::printf("R for roll 10, pitch -20, yaw 30 degrees:\n");
    for (const auto& row : R) {
        std::printf("  %9.5f %9.5f %9.5f\n", row[0], row[1], row[2]);
    }
    double r = 0, p = 0, y = 0;
    toEuler(R, r, p, y);
    std::printf("back to angles: roll %.3f, pitch %.3f, yaw %.3f degrees\n", r / deg, p / deg,
                y / deg);
    std::printf("thrust direction in the world (third column): (%.5f, %.5f, %.5f)\n", R[0][2],
                R[1][2], R[2][2]);
    std::printf("vertical part of thrust = cos(roll) cos(pitch) = %.5f\n",
                std::cos(roll) * std::cos(pitch));

    // Euler-angle rates from body rates (p, q, r): the formula of Layer 2, checked by
    // turning the body a tiny step and differencing the angles.
    const double bp = 0.3, bq = -0.2, br = 0.5;  // body rates, rad/s
    const double h = 1e-6;
    const double ang = std::sqrt(bp * bp + bq * bq + br * br) * h;
    const double ux = bp * h / ang, uy = bq * h / ang, uz = br * h / ang;
    const double c = std::cos(ang), s = std::sin(ang), t = 1 - c;
    const Mat3 step{{{t * ux * ux + c, t * ux * uy - s * uz, t * ux * uz + s * uy},
                     {t * ux * uy + s * uz, t * uy * uy + c, t * uy * uz - s * ux},
                     {t * ux * uz - s * uy, t * uy * uz + s * ux, t * uz * uz + c}}};
    double r2 = 0, p2 = 0, y2 = 0;
    toEuler(multiply(R, step), r2, p2, y2);  // body rates: the small turn multiplies on the right
    const double rollDot = bp + std::sin(roll) * std::tan(pitch) * bq
                           + std::cos(roll) * std::tan(pitch) * br;
    const double pitchDot = std::cos(roll) * bq - std::sin(roll) * br;
    const double yawDot = (std::sin(roll) * bq + std::cos(roll) * br) / std::cos(pitch);
    std::printf("Euler rates by formula   : %.6f %.6f %.6f rad/s\n", rollDot, pitchDot, yawDot);
    std::printf("Euler rates by difference: %.6f %.6f %.6f rad/s\n", (r2 - r) / h, (p2 - p) / h,
                (y2 - y) / h);

    std::printf("\nthe factor 1/cos(pitch) in the yaw-rate formula near pitch 90 degrees:\n");
    for (double pd : {0.0, 45.0, 80.0, 89.0, 89.9}) {
        std::printf("  pitch %5.1f deg: 1/cos(pitch) = %10.2f\n", pd, 1.0 / std::cos(pd * deg));
    }
    return 0;
}
