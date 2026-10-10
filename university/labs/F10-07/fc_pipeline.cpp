// F10-07 Listing 1: one pass through the data path inside a flight controller,
// sensor -> estimator -> rate controller -> mixer -> outputs, with the university's
// pretend numbers. The course quad numbering is the one of F10-02:
// 1 front-left (CW), 2 front-right (CCW), 3 rear-right (CW), 4 rear-left (CCW).
#include <algorithm>
#include <array>
#include <cstdio>

struct ImuSample
{
    double gx, gy, gz;   // rad/s, body rates (x forward, y left, z up)
};

struct Rates
{
    double p, q, r;      // estimated body rates, rad/s
};

// Estimator stage: here only a bias subtraction (DN301 replaces it with a real filter).
Rates estimate(const ImuSample& s, const Rates& bias)
{
    return {s.gx - bias.p, s.gy - bias.q, s.gz - bias.r};
}

// Rate controller: proportional only, gains are exercise values.
std::array<double, 3> rateControl(const Rates& want, const Rates& have)
{
    constexpr double kp = 0.15;
    return {kp * (want.p - have.p), kp * (want.q - have.q), kp * (want.r - have.r)};
}

// Mixer: thrust and three normalised torques -> four motor commands in 0..1.
std::array<double, 4> mix(double thrust, const std::array<double, 3>& u)
{
    constexpr std::array<double, 4> sRoll{+1, -1, -1, +1};
    constexpr std::array<double, 4> sPitch{-1, -1, +1, +1};
    constexpr std::array<double, 4> sYaw{+1, -1, +1, -1};
    std::array<double, 4> m{};
    for (int i = 0; i < 4; ++i) {
        m[i] = std::clamp(thrust + sRoll[i] * u[0] + sPitch[i] * u[1] + sYaw[i] * u[2], 0.0, 1.0);
    }
    return m;
}

// Output stage: a 0..1 command -> a pulse width for a pulse-type ESC input.
// The 1000..2000 us range is an exercise value (see the chapter's unverified box).
int toPulseUs(double cmd)
{
    return 1000 + static_cast<int>(cmd * 1000.0 + 0.5);
}

int main()
{
    const ImuSample s{0.20, -0.05, 0.01};          // the vehicle rolls right-wing-down
    const Rates bias{0.01, -0.01, 0.00};           // from a calibration at rest
    const Rates want{0.0, 0.0, 0.0};               // pilot: hold attitude rates at zero
    const double hoverThrust = 0.50;

    const Rates have = estimate(s, bias);
    std::printf("1 sensor     gyro  = %+.3f %+.3f %+.3f rad/s\n", s.gx, s.gy, s.gz);
    std::printf("2 estimator  rates = %+.3f %+.3f %+.3f rad/s (bias removed)\n",
                have.p, have.q, have.r);
    const auto u = rateControl(want, have);
    std::printf("3 controller u     = %+.4f %+.4f %+.4f (roll, pitch, yaw)\n", u[0], u[1], u[2]);
    const auto m = mix(hoverThrust, u);
    std::printf("4 mixer      motor = %.4f %.4f %.4f %.4f\n", m[0], m[1], m[2], m[3]);
    std::printf("5 outputs    pulse =");
    for (double v : m) {
        std::printf(" %d", toPulseUs(v));
    }
    std::printf(" us\n");
    return 0;
}
