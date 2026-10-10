// complementary.cpp - two complementary filters on the recorded IMU data of F10-13:
//   A. the angle-blending filter: integrate the gyro, then pull a little toward accel tilt;
//   B. a vector (Mahony-style) filter on a quaternion, with an optional gyro-bias integrator.
// Prints the RMS tilt error of each filter per part of the flight against the true attitude.
#include "../F10-04/quadsim.hpp"
#include "../F10-13/imu.hpp"

#include <array>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace {

// A. Angle-blending complementary filter. tau is the crossover time constant in seconds.
struct AngleFilter
{
    double tau;
    double roll = 0.0, pitch = 0.0;
    void update(const imu::Row& r, double dt)
    {
        const double sr = std::sin(roll), cr = std::cos(roll);
        roll += (r.gx + (r.gy * sr + r.gz * cr) * std::tan(pitch)) * dt; // gyro: fast path
        pitch += (r.gy * cr - r.gz * sr) * dt;
        const imu::Tilt a = imu::accelTilt(r.ax, r.ay, r.az); // accel: slow path
        const double alpha = tau / (tau + dt);
        roll = alpha * roll + (1.0 - alpha) * a.roll;
        pitch = alpha * pitch + (1.0 - alpha) * a.pitch;
    }
};

// B. Vector complementary filter: correct the gyro rate by kp times the rotation that
// would turn the estimated "up" direction into the measured one; ki integrates a bias.
struct VectorFilter
{
    double kp, ki;
    dn::Quat q;    // body to world
    dn::Vec3 bias; // estimated gyro bias, rad/s
    void update(const imu::Row& r, double dt)
    {
        const double n = std::sqrt(r.ax * r.ax + r.ay * r.ay + r.az * r.az);
        dn::Vec3 err{};
        if (n > 0.0) {
            const dn::Vec3 measured{r.ax / n, r.ay / n, r.az / n}; // "up", measured
            const dn::Quat qc{q.w, -q.x, -q.y, -q.z};
            const dn::Vec3 estimated = dn::rotate(qc, dn::Vec3{0.0, 0.0, 1.0}); // "up", estimated
            err = dn::cross(measured, estimated);
        }
        bias = bias - (ki * dt) * err;
        const dn::Vec3 w = dn::Vec3{r.gx, r.gy, r.gz} - bias + kp * err;
        const dn::Quat dq = dn::mul(q, dn::Quat{0.0, w.x, w.y, w.z});
        q = dn::normalized({q.w + 0.5 * dt * dq.w, q.x + 0.5 * dt * dq.x, q.y + 0.5 * dt * dq.y,
                            q.z + 0.5 * dt * dq.z});
    }
};

struct Score
{
    std::array<double, 6> sq{};
    std::array<int, 6> n{};
    void add(double t, double eRollDeg, double ePitchDeg)
    {
        const std::array<double, 7> edge{0.0, 2.0, 6.0, 13.0, 17.0, 21.0, 25.1};
        for (int i = 0; i < 6; ++i) {
            if (t > edge[i] && t <= edge[i + 1]) {
                sq[i] += eRollDeg * eRollDeg + ePitchDeg * ePitchDeg;
                ++n[i];
            }
        }
    }
    void print(const std::string& name) const
    {
        double all = 0.0;
        int count = 0;
        std::printf("%-22s", name.c_str());
        for (int i = 0; i < 6; ++i) {
            std::printf(" %6.2f", std::sqrt(sq[i] / n[i]));
            all += sq[i];
            count += n[i];
        }
        std::printf(" | %6.2f\n", std::sqrt(all / count));
    }
};

} // namespace

int main()
{
    const auto rows = imu::readCsv("../F10-13/imu_flight.csv");
    if (rows.size() < 2) {
        std::printf("cannot read ../F10-13/imu_flight.csv\n");
        return 1;
    }
    std::printf("RMS tilt error in degrees. Parts: 1 hover, 2 roll doublet, 3 accelerate/brake,\n");
    std::printf("4 yaw turn, 5 diagonal accel, 6 hover again.\n");
    std::printf("%-22s %6s %6s %6s %6s %6s %6s | %6s\n", "filter", "1", "2", "3", "4", "5", "6",
                "all");

    for (double tau : {0.0, 0.1, 0.3, 1.0, 3.0, 10.0, 1.0e9}) {
        AngleFilter f{tau};
        Score s;
        double tPrev = 0.0;
        for (const imu::Row& r : rows) {
            f.update(r, r.t - tPrev);
            tPrev = r.t;
            s.add(r.t, f.roll / imu::kDeg - r.roll, f.pitch / imu::kDeg - r.pitch);
        }
        const std::string name = tau == 0.0    ? "A accel only (tau 0)"
                                 : tau > 1.0e6 ? "A gyro only (tau inf)"
                                               : "A tau " + std::to_string(tau).substr(0, 4) + " s";
        s.print(name);
    }

    for (const auto& [kp, ki] : std::vector<std::array<double, 2>>{{1.0, 0.0}, {1.0, 0.05}}) {
        VectorFilter f{kp, ki, dn::Quat{}, dn::Vec3{}};
        Score s;
        double tPrev = 0.0;
        for (const imu::Row& r : rows) {
            f.update(r, r.t - tPrev);
            tPrev = r.t;
            const dn::Euler e = dn::toEuler(f.q);
            s.add(r.t, e.roll / imu::kDeg - r.roll, e.pitch / imu::kDeg - r.pitch);
        }
        char name[32];
        std::snprintf(name, sizeof name, "B kp %.1f ki %.2f", kp, ki);
        s.print(name);
        std::printf("    bias estimate after 25 s (rad/s): %.4f %.4f %.4f\n", f.bias.x, f.bias.y,
                    f.bias.z);
    }
    std::printf("true gyro bias in the recording (rad/s): 0.0040 -0.0060 0.0030\n");
    return 0;
}
