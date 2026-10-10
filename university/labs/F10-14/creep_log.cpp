// creep_log.cpp - F10-14 forensic evidence generator "The horizon creeps".
// The vehicle sits still and level on the bench for 120 s (propellers removed, motors off).
// The same angle-blending filter (tau 1 s, with an accelerometer gate) runs twice:
// with last week's firmware and with today's. One hidden change is revealed in the key.
#include "../F10-13/imu.hpp"

#include <cmath>
#include <cstdio>

namespace {

struct GatedFilter
{
    double tau = 1.0;
    double gateLow = 0.9 * imu::kG, gateHigh = 1.1 * imu::kG; // m/s^2: accept |f| near g
    double roll = 0.0, pitch = 0.0;
    long accepted = 0, seen = 0;
    void update(double gx, double gy, double gz, double ax, double ay, double az, double dt)
    {
        const double sr = std::sin(roll), cr = std::cos(roll);
        roll += (gx + (gy * sr + gz * cr) * std::tan(pitch)) * dt;
        pitch += (gy * cr - gz * sr) * dt;
        const double norm = std::sqrt(ax * ax + ay * ay + az * az);
        ++seen;
        if (norm > gateLow && norm < gateHigh) { // only trust "down" when |f| is close to g
            ++accepted;
            const imu::Tilt a = imu::accelTilt(ax, ay, az);
            const double alpha = tau / (tau + dt);
            roll = alpha * roll + (1.0 - alpha) * a.roll;
            pitch = alpha * pitch + (1.0 - alpha) * a.pitch;
        }
    }
};

void run(const char* title, double accelScale)
{
    imu::Rng rng(99);
    GatedFilter f;
    const double dt = 0.01;
    std::printf("%s\n", title);
    std::printf("  t(s)  |accel| as received  accepted(%%)  est roll  est pitch (deg)\n");
    double normSum = 0.0;
    for (int k = 1; k <= 12000; ++k) {
        const double gx = 0.004 + 0.003 * rng.gauss(), gy = -0.006 + 0.003 * rng.gauss();
        const double gz = 0.003 + 0.003 * rng.gauss();
        const double ax = (0.25 * rng.gauss()) * accelScale;
        const double ay = (0.25 * rng.gauss()) * accelScale;
        const double az = (imu::kG + 0.25 * rng.gauss()) * accelScale;
        f.update(gx, gy, gz, ax, ay, az, dt);
        normSum += std::sqrt(ax * ax + ay * ay + az * az);
        if (k % 2000 == 0) {
            std::printf("%6.0f  %12.3f %16.1f %11.2f %9.2f\n", k * dt, normSum / 2000.0,
                        100.0 * static_cast<double>(f.accepted) / static_cast<double>(f.seen),
                        f.roll / imu::kDeg, f.pitch / imu::kDeg);
            normSum = 0.0;
        }
    }
}

} // namespace

int main()
{
    run("Bench log 1 (last week's firmware), vehicle level and still:", 1.0);
    run("Bench log 2 (today's firmware, new IMU driver), vehicle level and still:", 1.0 / imu::kG);
    std::printf("Note from the update: 'New IMU driver, cleaner code, same sensor.'\n");
    return 0;
}
