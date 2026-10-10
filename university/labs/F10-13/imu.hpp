// imu.hpp - DN301 helpers shared by the estimation labs (F10-13, F10-14, F10-15).
// Axes as in DN201: body x forward, y left, z up; R = Rz(yaw) Ry(pitch) Rx(roll) (F10-03).
// An accelerometer measures specific force f = R^T (a - g_world), with g_world = (0, 0, -g),
// so a vehicle at rest and level reads f = (0, 0, +g).
#pragma once
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <numbers>
#include <string>
#include <vector>

namespace imu {

constexpr double kG = 9.81;                       // m/s^2, the value used in DN201 and DN301
constexpr double kDeg = std::numbers::pi / 180.0; // radians per degree

// One row of a recording: time, gyro (rad/s), accelerometer (m/s^2) and, because the
// recording comes from a simulator, the true attitude (degrees) for grading.
struct Row
{
    double t = 0.0;
    double gx = 0.0, gy = 0.0, gz = 0.0;
    double ax = 0.0, ay = 0.0, az = 0.0;
    double roll = 0.0, pitch = 0.0, yaw = 0.0;
};

// Reads the CSV written by record_flight.cpp. Returns an empty vector on any error.
inline std::vector<Row> readCsv(const std::string& path)
{
    std::vector<Row> rows;
    std::FILE* f = std::fopen(path.c_str(), "r");
    if (f == nullptr) {
        return rows;
    }
    char header[256];
    if (std::fgets(header, sizeof header, f) == nullptr) {
        std::fclose(f);
        return rows;
    }
    Row r;
    while (std::fscanf(f, "%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf", &r.t, &r.gx, &r.gy, &r.gz,
                       &r.ax, &r.ay, &r.az, &r.roll, &r.pitch, &r.yaw) == 10) {
        rows.push_back(r);
    }
    std::fclose(f);
    return rows;
}

// Tilt from one accelerometer sample, assuming the only specific force is "gravity's
// reaction" (no acceleration). Returns roll and pitch in radians.
struct Tilt
{
    double roll = 0.0, pitch = 0.0;
};
inline Tilt accelTilt(double ax, double ay, double az)
{
    return {std::atan2(ay, az), std::atan2(-ax, std::sqrt(ay * ay + az * az))};
}

inline double wrapPi(double a) // angle into [-pi, pi)
{
    return std::remainder(a, 2.0 * std::numbers::pi);
}

// Deterministic noise: a 64-bit linear congruential generator and Box-Muller.
// The same seed gives the same numbers with every compiler and library.
struct Rng
{
    std::uint64_t s;
    explicit Rng(std::uint64_t seed) : s(seed) {}
    double uniform() // in [0, 1)
    {
        s = s * 6364136223846793005ULL + 1442695040888963407ULL;
        return static_cast<double>(s >> 11) * 0x1.0p-53;
    }
    double gauss() // mean 0, standard deviation 1
    {
        const double r = std::sqrt(-2.0 * std::log(1.0 - uniform()));
        return r * std::cos(2.0 * std::numbers::pi * uniform());
    }
};

} // namespace imu
