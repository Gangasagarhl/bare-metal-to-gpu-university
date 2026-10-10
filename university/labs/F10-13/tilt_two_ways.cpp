// tilt_two_ways.cpp - attitude from the recorded IMU data (imu_flight.csv), two ways:
//   1. from the accelerometer alone, assuming it measures only "down";
//   2. from the gyro alone, integrating the Euler-angle rates from a known start (level).
// The recording comes from a simulator, so the true attitude is known and errors are exact.
#include "imu.hpp"

#include <array>
#include <cmath>
#include <cstdio>

int main()
{
    const auto rows = imu::readCsv("imu_flight.csv");
    if (rows.size() < 2) {
        std::printf("cannot read imu_flight.csv (run record_flight first)\n");
        return 1;
    }
    struct Segment
    {
        const char* name;
        double from, to;
        double accSq = 0.0, gyrSq = 0.0;
        int n = 0;
    };
    std::array<Segment, 6> seg{{{"hover", 0.0, 2.0},
                                {"roll doublet", 2.0, 6.0},
                                {"accelerate, brake", 6.0, 13.0},
                                {"yaw turn", 13.0, 17.0},
                                {"diagonal accel", 17.0, 21.0},
                                {"hover again", 21.0, 25.1}}};

    double roll = 0.0, pitch = 0.0; // gyro integration starts level (the vehicle was on the ground)
    double tPrev = 0.0;
    std::printf("  t(s) | true roll pitch | accel roll pitch | gyro roll pitch   (degrees)\n");
    for (const imu::Row& r : rows) {
        const double dt = r.t - tPrev;
        tPrev = r.t;
        // Euler-angle rates from body rates for R = Rz(yaw) Ry(pitch) Rx(roll) (F10-03).
        const double sr = std::sin(roll), cr = std::cos(roll);
        const double rollDot = r.gx + (r.gy * sr + r.gz * cr) * std::tan(pitch);
        const double pitchDot = r.gy * cr - r.gz * sr;
        roll += rollDot * dt;
        pitch += pitchDot * dt;

        const imu::Tilt a = imu::accelTilt(r.ax, r.ay, r.az);
        const double eA = std::hypot(a.roll / imu::kDeg - r.roll, a.pitch / imu::kDeg - r.pitch);
        const double eG = std::hypot(roll / imu::kDeg - r.roll, pitch / imu::kDeg - r.pitch);
        for (Segment& s : seg) {
            if (r.t > s.from && r.t <= s.to) {
                s.accSq += eA * eA;
                s.gyrSq += eG * eG;
                ++s.n;
            }
        }
        const long centi = std::lround(r.t * 100.0);
        if (centi % 100 == 0) {
            std::printf("%6.2f | %6.2f %6.2f | %7.2f %6.2f | %6.2f %6.2f\n", r.t, r.roll, r.pitch,
                        a.roll / imu::kDeg, a.pitch / imu::kDeg, roll / imu::kDeg,
                        pitch / imu::kDeg);
        }
    }
    std::printf("\nRMS tilt error (degrees) per part of the flight:\n");
    std::printf("  %-18s %8s %8s\n", "part", "accel", "gyro");
    for (const Segment& s : seg) {
        std::printf("  %-18s %8.2f %8.2f\n", s.name, std::sqrt(s.accSq / s.n),
                    std::sqrt(s.gyrSq / s.n));
    }
    return 0;
}
