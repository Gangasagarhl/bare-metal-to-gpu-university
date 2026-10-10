// checks.cpp - recomputes the numbers used in F10-19's text, worked example and answer key.
#include <cmath>
#include <cstdio>
#include <initializer_list>
#include <numbers>

int main()
{
    const double half = 1.576 - 1.516; // zero crossings of p in osc_log.out
    const double f = 1.0 / (2 * half);
    std::printf("W1 half period %.3f s -> %.2f Hz\n", half, f);
    for (double fr : {f, 10.5}) {
        std::printf("W2 phase lag of 6 ms at %.2f Hz: %.1f deg\n", fr, 360.0 * fr * 0.006);
    }
    std::printf(
        "W3 gain margins of P 20: delay 0 %.1f, 6 ms %.1f; flight P 120 is %.2f of Ku(6 ms)\n",
        975.3 / 20, 143.5 / 20, 120 / 143.5);
    const double deg = std::numbers::pi / 180.0;
    const double rateSp = 6.0 * 2 * std::sin(-4.0 * deg);
    std::printf("W4 rate setpoint on the 8 deg slope: %.3f rad/s\n", rateSp);
    std::printf("W5 integral after 4 s: ki e t = %.2f rad/s^2 -> %.4f N m\n", 5 * rateSp * 4,
                0.01 * 5 * rateSp * 4);
    std::printf("W6 run A minus run B torque at 4.00 s: %.4f N m\n", -0.3347 - -0.1675);
    std::printf("Q2 phase lag of 10 ms at 8.5 Hz: %.1f deg\n", 360.0 * 8.5 * 0.010);
    std::printf("Q6 half period 0.025 s -> %.1f Hz\n", 1.0 / (2 * 0.025));
    return 0;
}
