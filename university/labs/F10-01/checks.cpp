// checks.cpp - recomputes every number used in the text of F10-01 (worked example,
// questions, lab and forensic answers), so none of them is typed by hand.
#include <cmath>
#include <cstdio>
#include <initializer_list>
#include <numbers>

int main()
{
    const double g = 9.81, kT = 1.0e-5, maxThrust = 40.0, k = 0.05;
    const double rad2deg = 180.0 / std::numbers::pi;
    for (double m : {1.0, 1.25}) {
        const double w = m * g;
        std::printf("mass %.2f kg: weight %.4f N, per rotor %.4f N, rotor speed %.1f rad/s, "
                    "T/W %.3f, steepest tilt that keeps height %.1f deg\n",
                    m, w, w / 4, std::sqrt(w / 4 / kT), maxThrust / w,
                    std::acos(w / maxThrust) * rad2deg);
    }
    for (double ratio : {1.1, 1.2, 1.5}) {
        std::printf("thrust/weight %.1f (1.0 kg): terminal climb speed %.3f m/s\n", ratio,
                    std::sqrt((ratio - 1.0) * g / k));
    }
    std::printf("thrust/weight 1.2 (1.25 kg): terminal climb speed %.3f m/s\n",
                std::sqrt(0.2 * 1.25 * g / k));
    std::printf("hexacopter 3.0 kg, six rotors of 8 N: T/W %.3f, per rotor %.3f N\n",
                48.0 / (3.0 * g), 3.0 * g / 6.0);
    std::printf("forensic: P-only height loop, kp 4 N/m, feedforward 9.81 N, mass 1.25 kg: "
                "height error %.4f m\n", (1.25 * g - 9.81) / 4.0);
    std::printf("forensic: same with mass 1.10 kg: height error %.4f m\n", (1.10 * g - 9.81) / 4.0);
    std::printf("tilt 25 deg, mass 1.0 kg: thrust %.3f N, sideways accel %.3f m/s^2\n",
                g / std::cos(25.0 / rad2deg), g * std::tan(25.0 / rad2deg));
    std::printf("start of climb at 1.2 m g: a = %.3f m/s^2\n", 0.2 * g);
    return 0;
}
