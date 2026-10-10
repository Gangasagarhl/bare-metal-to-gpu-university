// flight_sim.cc - simulates a hover with a yaw turn, runs a small position estimator
// that fuses GNSS with an innovation gate, and writes everything to a ULog-style log.
//   flight_sim <out.ulg> <UNI_GPS_OFF_X value in metres>
// The GNSS antenna is really mounted 0.15 m forward of the IMU. The estimator uses the
// parameter UNI_GPS_OFF_X (this course's name, not PX4's) to correct for that lever arm.
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "ulog_lite.h"

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kTrueOffX = 0.15;   // where the antenna really is (m, forward of the IMU)

class Rng   // deterministic: the same log every run
{
public:
    double gauss()
    {
        const double u1 = (static_cast<double>(next() >> 11) + 1.0) / 9007199254740993.0;
        const double u2 = static_cast<double>(next() >> 11) / 9007199254740992.0;
        return std::sqrt(-2.0 * std::log(u1)) * std::cos(2.0 * kPi * u2);
    }

private:
    std::uint64_t next()
    {
        s_ = s_ * 6364136223846793005ULL + 1442695040888963407ULL;
        return s_;
    }
    std::uint64_t s_ = 2026;
};

struct Axis   // one horizontal axis of the estimator: position and velocity
{
    double p = 0.0, v = 0.0;
    double P00 = 1.0, P01 = 0.0, P11 = 1.0;

    void predict(double a, double dt, double q)
    {
        p += v * dt + 0.5 * a * dt * dt;
        v += a * dt;
        const double n00 = P00 + dt * (2.0 * P01 + dt * P11) + q * dt * dt * dt * dt / 4.0;
        const double n01 = P01 + dt * P11 + q * dt * dt * dt / 2.0;
        const double n11 = P11 + q * dt * dt;
        P00 = n00;
        P01 = n01;
        P11 = n11;
    }

    void fuse(double innov, double r)
    {
        const double s = P00 + r;
        const double k0 = P00 / s;
        const double k1 = P01 / s;
        p += k0 * innov;
        v += k1 * innov;
        const double n00 = (1.0 - k0) * P00;
        const double n01 = (1.0 - k0) * P01;
        const double n11 = P11 - k1 * P01;
        P00 = n00;
        P01 = n01;
        P11 = n11;
    }
};

double yawDeg(double t)   // face north; turn to south at 90 deg/s; hold; turn back; hold
{
    if (t < 10.0) {
        return 0.0;
    }
    if (t < 12.0) {
        return 90.0 * (t - 10.0);
    }
    if (t < 22.0) {
        return 180.0;
    }
    if (t < 24.0) {
        return 180.0 - 90.0 * (t - 22.0);
    }
    return 0.0;
}

} // namespace

int main(int argc, char** argv)
{
    if (argc != 3) {
        std::fprintf(stderr, "usage: flight_sim <out.ulg> <UNI_GPS_OFF_X>\n");
        return 2;
    }
    const float offX = std::strtof(argv[2], nullptr);
    const float gate = 4.0f;        // innovation gate, in standard deviations
    const float gpsNoise = 0.30f;   // assumed GNSS horizontal noise, m (1 sigma)
    const float accNoise = 0.15f;   // assumed accelerometer noise, m/s^2 (1 sigma)
    const float timeout = 5.0f;     // s without fusion before a reset to GNSS

    ulog_lite::Writer w(argv[1], 0);
    w.format("vehicle_state_model:uint64_t timestamp;float yaw_deg;float true_n;float true_e");
    w.format("gnss_model:uint64_t timestamp;float ant_n;float ant_e");
    w.format("est_innov_model:uint64_t timestamp;float innov_n;float innov_e;float test_ratio;uint8_t fused");
    w.format("est_pos_model:uint64_t timestamp;float pos_n;float pos_e;float vel_n;float vel_e");
    w.info("sys_name", "dn302-flight-sim");
    w.info("ver_note", "course model, not PX4");
    w.param("UNI_ACC_NOISE", accNoise);
    w.param("UNI_BARO_OFF_Z", 0.0f);
    w.param("UNI_GATE", gate);
    w.param("UNI_GPS_NOISE", gpsNoise);
    w.param("UNI_GPS_OFF_X", offX);
    w.param("UNI_GPS_OFF_Y", 0.0f);
    w.param("UNI_GPS_TOUT", timeout);
    w.param("UNI_IMU_OFF_X", 0.0f);
    const auto idState = w.addLogged("vehicle_state_model");
    const auto idGnss = w.addLogged("gnss_model");
    const auto idInnov = w.addLogged("est_innov_model");
    const auto idPos = w.addLogged("est_pos_model");

    Rng rng;
    Axis n;
    Axis e;
    const double dt = 0.01;
    double lastFuse = 0.0;
    bool fusing = true;
    int rejected = 0;
    int fusedCount = 0;
    int resets = 0;
    double maxRatio = 0.0;
    double maxErr = 0.0;
    for (int k = 1; k <= 3400; ++k) {   // 34 s at 100 Hz
        const double t = k * dt;
        const auto tUs = static_cast<std::uint64_t>(k) * 10000;
        const double psi = yawDeg(t) * kPi / 180.0;
        // Truth: the vehicle hovers at the origin. Measured acceleration = noise only.
        n.predict(accNoise * rng.gauss(), dt, accNoise * accNoise);
        e.predict(accNoise * rng.gauss(), dt, accNoise * accNoise);
        if (k % 20 == 0) {   // GNSS at 5 Hz
            const double antN = kTrueOffX * std::cos(psi) + gpsNoise * rng.gauss();
            const double antE = kTrueOffX * std::sin(psi) + gpsNoise * rng.gauss();
            w.data(idGnss, (ulog_lite::Pack() << tUs << static_cast<float>(antN) << static_cast<float>(antE)).str());
            // Predicted antenna position = estimated IMU position + yaw-rotated lever arm.
            const double inN = antN - (n.p + offX * std::cos(psi));
            const double inE = antE - (e.p + offX * std::sin(psi));
            const double r = static_cast<double>(gpsNoise) * gpsNoise;
            const double g2 = static_cast<double>(gate) * gate;
            const double ratio = std::fmax(inN * inN / (g2 * (n.P00 + r)), inE * inE / (g2 * (e.P00 + r)));
            maxRatio = std::fmax(maxRatio, ratio);
            const bool ok = ratio <= 1.0;
            if (ok) {
                n.fuse(inN, r);
                e.fuse(inE, r);
                lastFuse = t;
                ++fusedCount;
                if (!fusing) {
                    w.text(6, tUs, "GNSS fusion resumed");
                }
            } else {
                ++rejected;
                if (fusing) {
                    char msg[80];
                    std::snprintf(msg, sizeof msg, "GNSS fusion stopped: test ratio %.2f", ratio);
                    w.text(4, tUs, msg);
                }
                if (t - lastFuse >= timeout) {   // reset the position to what GNSS says
                    n.p = antN - offX * std::cos(psi);
                    e.p = antE - offX * std::sin(psi);
                    n.P00 = e.P00 = r;
                    lastFuse = t;
                    ++resets;
                    w.text(4, tUs, "position reset to GNSS after timeout");
                }
            }
            fusing = ok;
            w.data(idInnov, (ulog_lite::Pack() << tUs << static_cast<float>(inN) << static_cast<float>(inE)
                                                << static_cast<float>(ratio) << static_cast<std::uint8_t>(ok))
                                .str());
        }
        if (k % 10 == 0) {   // 10 Hz
            w.data(idState, (ulog_lite::Pack() << tUs << static_cast<float>(yawDeg(t)) << 0.0f << 0.0f).str());
            w.data(idPos, (ulog_lite::Pack() << tUs << static_cast<float>(n.p) << static_cast<float>(e.p)
                                              << static_cast<float>(n.v) << static_cast<float>(e.v))
                              .str());
            maxErr = std::fmax(maxErr, std::hypot(n.p, e.p));
        }
    }
    std::printf("%s: UNI_GPS_OFF_X=%.2f m, GNSS samples fused %d, rejected %d, resets %d, "
                "max test ratio %.2f, max position error %.2f m\n",
                argv[1], static_cast<double>(offX), fusedCount, rejected, resets, maxRatio, maxErr);
    return 0;
}
