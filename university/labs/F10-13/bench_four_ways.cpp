// bench_four_ways.cpp - F10-13 forensic evidence generator "It drifts left in every hover".
// A vehicle stands on a bench table (propellers removed, motors off) and is turned by hand
// to four headings; at each heading the accelerometer is logged for 5 s at 100 Hz.
// Synthetic data from a model whose two hidden rotations are revealed in the answer key.
#include "imu.hpp"

#include <array>
#include <cmath>
#include <cstdio>

namespace {

using Vec = std::array<double, 3>;
using Mat = std::array<Vec, 3>;

Mat mul(const Mat& a, const Mat& b)
{
    Mat c{};
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            for (int k = 0; k < 3; ++k) c[i][j] += a[i][k] * b[k][j];
    return c;
}
Mat transpose(const Mat& a)
{
    Mat t{};
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j) t[i][j] = a[j][i];
    return t;
}
Vec rotate(const Mat& a, const Vec& v)
{
    return {a[0][0] * v[0] + a[0][1] * v[1] + a[0][2] * v[2],
            a[1][0] * v[0] + a[1][1] * v[1] + a[1][2] * v[2],
            a[2][0] * v[0] + a[2][1] * v[1] + a[2][2] * v[2]};
}
Mat rotZYX(double roll, double pitch, double yaw) // R = Rz Ry Rx, angles in degrees
{
    const double r = roll * imu::kDeg, p = pitch * imu::kDeg, y = yaw * imu::kDeg;
    const Mat Rx{{{1, 0, 0}, {0, std::cos(r), -std::sin(r)}, {0, std::sin(r), std::cos(r)}}};
    const Mat Ry{{{std::cos(p), 0, std::sin(p)}, {0, 1, 0}, {-std::sin(p), 0, std::cos(p)}}};
    const Mat Rz{{{std::cos(y), -std::sin(y), 0}, {std::sin(y), std::cos(y), 0}, {0, 0, 1}}};
    return mul(Rz, mul(Ry, Rx));
}

} // namespace

int main()
{
    const Mat table = rotZYX(0.4, 0.45, 0.0); // hidden: the table is not quite level
    const Mat mount = rotZYX(2.5, -0.8, 0.0); // hidden: IMU board relative to the frame
    imu::Rng rng(4242);
    const double noise = 0.25; // m/s^2 per sample, as in imu_flight.csv

    std::printf("Bench log: vehicle on table, turned by hand; 500 samples per heading\n");
    std::printf("heading | mean ax    ay      az    (m/s^2) | accel roll  pitch (deg) | std ax\n");
    for (double heading : {0.0, 90.0, 180.0, 270.0}) {
        const Mat vehicle = mul(table, rotZYX(0.0, 0.0, heading));
        const Mat imuToWorld = mul(vehicle, mount);
        const Vec f = rotate(transpose(imuToWorld), Vec{0.0, 0.0, imu::kG});
        Vec sum{}, sumSq{};
        for (int k = 0; k < 500; ++k) {
            for (int i = 0; i < 3; ++i) {
                const double v = f[i] + noise * rng.gauss();
                sum[i] += v;
                sumSq[i] += v * v;
            }
        }
        const Vec m{sum[0] / 500, sum[1] / 500, sum[2] / 500};
        const double sx = std::sqrt(sumSq[0] / 500 - m[0] * m[0]);
        const imu::Tilt t = imu::accelTilt(m[0], m[1], m[2]);
        std::printf("%6.0f  | %7.4f %7.4f %7.4f       | %8.3f %8.3f       | %.3f\n", heading, m[0],
                    m[1], m[2], t.roll / imu::kDeg, t.pitch / imu::kDeg, sx);
    }
    std::printf("Pilot's note: 'Drifts left in every hover; I hold right stick to stay put.'\n");
    return 0;
}
