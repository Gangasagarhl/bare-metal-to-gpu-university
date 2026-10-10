// record_flight.cpp - makes the DN301 "recorded IMU data": a 25 s flight of the course quad
// in the DN201 simulator (quadsim.hpp, F10-04), sampled by a modelled IMU at 100 Hz.
// Writes imu_flight.csv (gyro, accelerometer and the true attitude for grading) and
// gnss_flight.csv (a modelled GNSS receiver at 10 Hz, with the true position and velocity).
// The IMU model (bias, noise, 10 ms averaging) is an exercise model, not a real part.
#include "../F10-04/quadsim.hpp"
#include "imu.hpp"

#include <algorithm>
#include <cstdio>

namespace {

struct Setpoint
{
    double roll = 0.0, pitch = 0.0, yawRate = 0.0; // rad, rad, rad/s
};

// The scripted manoeuvres, in degrees and degrees per second.
Setpoint script(double t)
{
    Setpoint s;
    if (t >= 2.0 && t < 3.0) s.roll = 20.0; // roll doublet
    if (t >= 3.0 && t < 4.0) s.roll = -20.0;
    if (t >= 6.0 && t < 9.0) s.pitch = 12.0;     // nose down: accelerate forward
    if (t >= 9.0 && t < 11.0) s.pitch = -12.0;   // nose up: brake
    if (t >= 13.0 && t < 15.0) s.yawRate = 45.0; // turn left by 90 degrees
    if (t >= 17.0 && t < 19.0) {
        s.roll = 15.0;
        s.pitch = 10.0;
    } // diagonal acceleration
    return {s.roll * imu::kDeg, s.pitch * imu::kDeg, s.yawRate * imu::kDeg};
}

} // namespace

int main()
{
    const dn::Params P;
    dn::State s;
    const double w0 = dn::hoverRotorSpeed(P);
    s.rotor = {w0, w0, w0, w0};
    const double h = 0.001;                    // simulator step and controller period, s
    const int perSample = 10;                  // one IMU row every 10 steps = 100 Hz
    const dn::Vec3 bias{0.004, -0.006, 0.003}; // gyro bias, rad/s (exercise values)
    const double gyroNoise = 0.003;            // rad/s, standard deviation per row
    const double accNoise = 0.25;              // m/s^2, standard deviation per row
    imu::Rng rng(20261010);

    std::FILE* out = std::fopen("imu_flight.csv", "w");
    if (out == nullptr) {
        std::printf("cannot write imu_flight.csv\n");
        return 1;
    }
    std::fprintf(out, "t_s,gx,gy,gz,ax,ay,az,roll_deg,pitch_deg,yaw_deg\n");
    std::FILE* gnss = std::fopen("gnss_flight.csv", "w");
    if (gnss == nullptr) {
        std::fclose(out);
        std::printf("cannot write gnss_flight.csv\n");
        return 1;
    }
    std::fprintf(gnss, "t_s,px,py,pz,vx,vy,vz,true_px,true_py,true_pz,true_vx,true_vy,true_vz\n");
    const double posNoise = 0.5, velNoise = 0.1; // m and m/s per GNSS row (exercise values)

    Setpoint smooth;     // setpoints after a 0.15 s first-order smoothing
    dn::Vec3 sumW, sumF; // sums over one 10 ms sample window
    int rows = 0;
    double maxTilt = 0.0, maxSpeed = 0.0;
    for (int k = 1; k <= 25000; ++k) {
        const double t = k * h;
        const Setpoint target = script(t);
        const double a = h / 0.15;
        smooth.roll += a * (target.roll - smooth.roll);
        smooth.pitch += a * (target.pitch - smooth.pitch);
        smooth.yawRate += a * (target.yawRate - smooth.yawRate);

        // The "test pilot": attitude PD and yaw-rate P on the true state, height held by
        // a vertical-speed P term. F10-16 builds a real cascaded controller.
        const dn::Euler e = dn::toEuler(s.q);
        const double kp = 60.0, kd = 12.0;
        const dn::Vec3 J = P.inertia;
        dn::Vec3 tau{J.x * (kp * (smooth.roll - e.roll) - kd * s.w.x),
                     J.y * (kp * (smooth.pitch - e.pitch) - kd * s.w.y),
                     J.z * 8.0 * (smooth.yawRate - s.w.z)};
        const double thrust = P.mass * (P.g - 2.0 * s.v.z) / (std::cos(e.roll) * std::cos(e.pitch));
        const dn::Rotors cmd = dn::mix(P, thrust, tau);

        // True specific force in the body frame: f = R^T (dv/dt - g_world).
        const dn::Deriv d = dn::derivative(P, s, cmd);
        const dn::Quat qc{s.q.w, -s.q.x, -s.q.y, -s.q.z};
        const dn::Vec3 fBody = dn::rotate(qc, d.dv + dn::Vec3{0.0, 0.0, P.g});
        dn::rk4Step(P, s, cmd, h);
        sumW = sumW + s.w;
        sumF = sumF + fBody;

        if (k % perSample == 0) {
            const double n = perSample;
            const dn::Euler te = dn::toEuler(s.q);
            std::fprintf(out, "%.2f,%.5f,%.5f,%.5f,%.4f,%.4f,%.4f,%.3f,%.3f,%.3f\n", t,
                         sumW.x / n + bias.x + gyroNoise * rng.gauss(),
                         sumW.y / n + bias.y + gyroNoise * rng.gauss(),
                         sumW.z / n + bias.z + gyroNoise * rng.gauss(),
                         sumF.x / n + accNoise * rng.gauss(), sumF.y / n + accNoise * rng.gauss(),
                         sumF.z / n + accNoise * rng.gauss(), te.roll / imu::kDeg,
                         te.pitch / imu::kDeg, te.yaw / imu::kDeg);
            sumW = {};
            sumF = {};
            ++rows;
            if (k % 100 == 0) { // GNSS at 10 Hz
                std::fprintf(gnss,
                             "%.1f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f\n",
                             t, s.p.x + posNoise * rng.gauss(), s.p.y + posNoise * rng.gauss(),
                             s.p.z + posNoise * rng.gauss(), s.v.x + velNoise * rng.gauss(),
                             s.v.y + velNoise * rng.gauss(), s.v.z + velNoise * rng.gauss(), s.p.x,
                             s.p.y, s.p.z, s.v.x, s.v.y, s.v.z);
            }
            maxTilt = std::max({maxTilt, std::abs(te.roll), std::abs(te.pitch)});
            maxSpeed = std::max(maxSpeed, std::sqrt(dn::dot(s.v, s.v)));
        }
    }
    std::fclose(out);
    std::fclose(gnss);
    std::printf("wrote imu_flight.csv: %d rows, 100 Hz, 25 s; gnss_flight.csv: 10 Hz\n", rows);
    std::printf(
        "gyro bias (rad/s): %.3f %.3f %.3f; gyro noise %.3f rad/s; accel noise %.2f m/s^2\n",
        bias.x, bias.y, bias.z, gyroNoise, accNoise);
    std::printf("GNSS noise: position %.1f m, velocity %.1f m/s (each axis)\n", posNoise, velNoise);
    std::printf("largest tilt %.1f deg, largest speed %.2f m/s\n", maxTilt / imu::kDeg, maxSpeed);
    std::printf("final position (m): x %.2f y %.2f z %.2f; final yaw %.1f deg\n", s.p.x, s.p.y,
                s.p.z, dn::toEuler(s.q).yaw / imu::kDeg);
    return 0;
}
