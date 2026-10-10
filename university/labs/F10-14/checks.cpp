// checks.cpp - recomputes the numbers used in F10-14's text, worked example and answer key.
#include <cmath>
#include <cstdio>
#include <initializer_list>
#include <numbers>

int main()
{
    const double deg = std::numbers::pi / 180.0, dt = 0.01;
    for (double tau : {0.1, 1.0, 3.0}) {
        std::printf("W1 tau %.1f s, dt 0.01 s: alpha = %.6f, crossover %.3f rad/s = %.4f Hz\n", tau,
                    tau / (tau + dt), 1.0 / tau, 1.0 / (2 * std::numbers::pi * tau));
    }
    std::printf("W2 steady error tau*b, tau 1 s: b 0.004 -> %.3f deg; b -0.006 -> %.3f deg\n",
                1.0 * 0.004 / deg, -1.0 * 0.006 / deg);
    const double sAcc = std::atan2(0.25, 9.81) / deg;
    std::printf("W3 accel tilt noise %.3f deg; after the low-pass (tau 1 s): about %.3f deg\n",
                sAcc, sAcc * std::sqrt(dt / (2 * 1.0)));
    std::printf("W4 gate in m/s^2: %.3f to %.3f; a reading in g: 1.000\n", 0.9 * 9.81, 1.1 * 9.81);
    std::printf("W5 gyro-only drift 120 s: roll %.1f deg (b 0.004), pitch %.1f deg (b -0.006)\n",
                0.004 * 120 / deg, -0.006 * 120 / deg);
    // One step of the filter by hand.
    const double prev = 2.0 * deg, gyro = 0.10, accTilt = 3.0 * deg, tau = 1.0;
    const double a = tau / (tau + dt);
    const double pred = prev + gyro * dt;
    const double out = a * pred + (1 - a) * accTilt;
    std::printf("W6 one step: predicted %.4f deg, blended %.4f deg\n", pred / deg, out / deg);
    std::printf("Q2 tau 0.5 s, dt 0.004 s: alpha = %.5f\n", 0.5 / (0.5 + 0.004));
    std::printf("Q4 tau 2 s, b 0.003 rad/s: steady error %.4f rad = %.2f deg\n", 2 * 0.003,
                2 * 0.003 / deg);
    return 0;
}
