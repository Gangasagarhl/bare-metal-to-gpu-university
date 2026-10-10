// checks.cpp - recomputes the numbers used in F10-17's text, worked example and answer key.
#include <cmath>
#include <cstdio>
#include <initializer_list>

int main()
{
    const double kT = 1e-5, arm = 0.15, Jx = 0.010, g = 9.81, w0 = std::sqrt(g / (4 * kT));
    const double df = 0.05 * g / 4, tq = arm * df, acc = tq / Jx;
    std::printf("W1 5 %% stronger propeller on motor 1: extra force %.4f N, roll torque %.5f N m, "
                "angular accel %.3f rad/s^2\n",
                df, tq, acc);
    for (double kp : {5.0, 10.0, 20.0, 60.0}) {
        std::printf("W2 P only, kp %4.0f: steady rate error %.3f rad/s\n", kp, acc / kp);
    }
    const double sigma = 0.02, dt = 0.002;
    for (double kd : {0.3, 1.2}) {
        std::printf("W3 D unfiltered, kd %.1f: torque noise J kd sqrt2 sigma/dt = %.4f N m\n", kd,
                    Jx * kd * std::sqrt(2.0) * sigma / dt);
    }
    std::printf("W4 P noise, kp 20: J kp sigma = %.4f N m\n", Jx * 20 * sigma);
    const double tqNoise = Jx * 1.2 * std::sqrt(2.0) * sigma / dt;
    const double dfm = tqNoise / (4 * arm), dw = dfm / (2 * kT * w0);
    std::printf("W5 hover speed %.1f rad/s; roll torque noise %.4f N m -> motor speed noise %.1f "
                "rad/s (roll only), %.1f with pitch as well\n",
                w0, tqNoise, dw, dw * std::sqrt(2.0));
    std::printf("W6 time for I to cancel the disturbance at error 0.09 rad/s: ki 5 -> %.1f s, "
                "ki 40 -> %.2f s\n",
                acc / (5 * 0.09), acc / (40 * 0.09));
    std::printf("Q2 P only, kp 30: steady rate error %.4f rad/s\n", acc / 30);
    std::printf("Q4 D unfiltered, kd 0.3, loop 1 kHz: torque noise %.4f N m (500 Hz: %.4f)\n",
                Jx * 0.3 * std::sqrt(2.0) * sigma / 0.001, Jx * 0.3 * std::sqrt(2.0) * sigma / dt);
    std::printf(
        "Q6 motor time constant 0.03 s: corner frequency %.1f Hz; D filter 0.005 s: %.1f Hz\n",
        1 / (2 * 3.14159265358979 * 0.03), 1 / (2 * 3.14159265358979 * 0.005));
    return 0;
}
