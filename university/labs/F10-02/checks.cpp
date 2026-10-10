// checks.cpp - recomputes the numbers used in the text of F10-02 (worked example, questions,
// forensic answer key) from the course quad's constants and from values printed in the logs.
#include <cmath>
#include <cstdio>
#include <initializer_list>

int main()
{
    const double kT = 1.0e-5, kQ = 1.5e-7, arm = 0.15, c = kQ / kT;
    std::printf("c = kQ/kT = %.4f m; arm / c = %.1f\n", c, arm / c);
    std::printf("roll 0.1 N m: force change per rotor %.4f N\n", 0.1 / (4 * arm));
    std::printf("yaw 0.01 N m: force change per rotor %.4f N\n", 0.01 / (4 * c));
    for (double f : {2.4525, 2.6192, 2.2858}) {
        std::printf("force %.4f N -> rotor speed %.1f rad/s\n", f, std::sqrt(f / kT));
    }
    std::printf("largest yaw torque at hover before a rotor reaches zero: %.4f N m\n",
                4 * c * 2.4525);
    // forensic: settled speeds printed in yaw_climb.out at 3.50 s (today's flight)
    const double w1 = 475.9, w2 = 514.1, w3 = 434.4, w4 = 514.1;
    std::printf("today, if all props were normal: f1 %.3f f2 %.3f f3 %.3f f4 %.3f N\n",
                kT * w1 * w1, kT * w2 * w2, kT * w3 * w3, kT * w4 * w4);
    std::printf("today, motor 3 with 1.2 kT: f3 = %.3f N\n", 1.2 * kT * w3 * w3);
    std::printf("yaw torques if all normal: CW %.5f CCW %.5f N m\n",
                kQ * (w1 * w1 + w3 * w3), kQ * (w2 * w2 + w4 * w4));
    std::printf("yaw torques with motor 3 at 1.6 kQ: CW %.5f CCW %.5f N m\n",
                kQ * (w1 * w1 + 1.6 * w3 * w3), kQ * (w2 * w2 + w4 * w4));
    std::printf("speed ratio w2/w1 at 3.5 s: %.3f; at 6.0 s: %.3f\n", w2 / w1, 581.6 / 539.1);
    std::printf("speed gap w2-w1 at 3.5 s: %.1f rad/s; at 6.0 s: %.1f rad/s\n", w2 - w1,
                581.6 - 539.1);
    return 0;
}
