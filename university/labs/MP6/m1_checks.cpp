// m1_checks.cpp - MP6 handbook, worked example: the numbers computed in the text, recomputed.
// Inputs are the lab plan's values and one row of m1_log.csv (t = 20.00 s, the e-stop moment of B1).
#include <cmath>
#include <cstdio>

int main()
{
    const double a = 0.8;          // m/s^2, the drive's ramp (mp6_robot.hpp)
    const double b = 0.300;        // m, track
    const double dt = 0.01;        // s, supervisor and physics step
    const double v = 0.300;        // m/s, v_cmd in m1_log.csv at t = 20.00
    const double w = 0.573;        // rad/s, w_cmd in m1_log.csv at t = 20.00

    std::printf("1. straight line at %.3f m/s: stop time %.4f s, distance %.4f m\n", v, v / a, v * v / (2.0 * a));
    const double vl = v - 0.5 * w * b;
    const double vr = v + 0.5 * w * b;
    std::printf("2. turning: wheel speeds %.4f and %.4f m/s\n", vl, vr);
    std::printf("   stop time (faster wheel) %.4f s\n", std::fmax(vl, vr) / a);
    std::printf("   centre distance, continuous ramp: %.4f m\n", (vl * vl + vr * vr) / (4.0 * a));
    double dl = 0.0, dr = 0.0, sl = vl, sr = vr;     // the same ramp in 10 ms steps, as the simulator does
    while (sl > 0.0 || sr > 0.0) {
        sl = std::fmax(0.0, sl - a * dt);
        sr = std::fmax(0.0, sr - a * dt);
        dl += sl * dt;
        dr += sr * dt;
    }
    std::printf("   centre distance, 10 ms steps: %.4f m\n", 0.5 * (dl + dr));
    const double g = -2.0 * std::log(0.01);
    std::printf("3. NIS gate for 2 readings at 1 %% false rejection: %.4f; P(NIS > 9.21) = %.5f\n", g,
                std::exp(-9.21 / 2.0));
    const int seen = 170 + 166 + 149 + 67 + 79 + 117 + 91 + 12;   // m1_forensic_fix.out, run 2
    std::printf("   run 2 of m1_forensic_fix saw %d readings: about %.1f false rejections expected\n", seen, 0.01 * seen);
    const double wd = 0.25;
    std::printf("4. watchdog: worst detection %.2f s after the last command; wheels still by %.3f s after it\n",
                wd + dt, wd + dt + std::fmax(vl, vr) / a);
    return 0;
}
