// checks.cpp - recomputes the numbers used in F10-13's text, worked example and answer key.
#include <cmath>
#include <cstdio>
#include <numbers>

int main()
{
    const double g = 9.81, deg = std::numbers::pi / 180.0;
    // Worked example: one accelerometer sample.
    const double fx = -1.20, fy = 1.65, fz = 9.60;
    const double roll = std::atan2(fy, fz), pitch = std::atan2(-fx, std::hypot(fy, fz));
    std::printf("W1 sample (%.2f, %.2f, %.2f): |f| %.3f m/s^2, roll %.2f deg, pitch %.2f deg\n", fx,
                fy, fz, std::sqrt(fx * fx + fy * fy + fz * fz), roll / deg, pitch / deg);
    // A multirotor tilted 12 degrees, accelerating without drag, holding height.
    const double tilt = 12.0 * deg;
    std::printf("W2 tilt 12 deg: horizontal accel g tan = %.3f m/s^2; accelerometer reads "
                "(0, 0, %.3f) -> accel tilt 0 deg\n",
                g * std::tan(tilt), g / std::cos(tilt));
    // The same vehicle at a steady speed with drag: specific force = -g_world in the body frame.
    std::printf("W3 steady flight at 12 deg: f_body = (%.3f, 0, %.3f) -> accel pitch %.2f deg\n",
                -g * std::sin(tilt), g * std::cos(tilt),
                std::atan2(g * std::sin(tilt), g * std::cos(tilt)) / deg);
    // Gyro bias integrated for 25 s.
    std::printf("W4 bias -0.006 rad/s for 25 s: %.3f rad = %.2f deg\n", -0.006 * 25,
                -0.006 * 25 / deg);
    // Accelerometer noise 0.25 m/s^2 as an angle.
    std::printf("W5 accel noise 0.25 m/s^2 as tilt noise: %.2f deg\n", std::atan2(0.25, g) / deg);
    // Forensic: four headings (values copied from bench_four_ways.out).
    const double r[4] = {2.862, 2.969, 2.136, 2.085}, p[4] = {-0.323, -1.316, -1.228, -0.426};
    const double rm = (r[0] + r[1] + r[2] + r[3]) / 4, pm = (p[0] + p[1] + p[2] + p[3]) / 4;
    std::printf("K1 mean over headings: roll %.3f deg, pitch %.3f deg (mounting offset)\n", rm, pm);
    std::printf("K2 half-differences 0/180: roll %.3f pitch %.3f; 90/270: roll %.3f pitch %.3f\n",
                (r[0] - r[2]) / 2, (p[0] - p[2]) / 2, (r[1] - r[3]) / 2, (p[1] - p[3]) / 2);
    std::printf("K3 table tilt size from heading 0: %.3f deg\n",
                std::hypot((r[0] - r[2]) / 2, (p[0] - p[2]) / 2));
    std::printf("K4 true roll when the estimate says level: %.2f deg; sideways accel %.3f m/s^2\n",
                -rm, g * std::tan(rm * deg));
    // Check-yourself answers.
    const double qx = 0.85, qy = -0.50, qz = 9.76;
    std::printf("Q2 (%.2f, %.2f, %.2f): roll %.2f deg, pitch %.2f deg, |f| %.3f\n", qx, qy, qz,
                std::atan2(qy, qz) / deg, std::atan2(-qx, std::hypot(qy, qz)) / deg,
                std::sqrt(qx * qx + qy * qy + qz * qz));
    std::printf("Q4 bias 0.004 rad/s for 60 s: %.3f rad = %.2f deg\n", 0.004 * 60,
                0.004 * 60 / deg);
    // Lab expectations.
    std::printf("L1 standard error of a 200-sample gyro mean: %.5f rad/s\n",
                0.003 / std::sqrt(200.0));
    std::printf("L2 |f| - g at a held 20 deg roll: %.3f m/s^2\n", g / std::cos(20 * deg) - g);
    return 0;
}
