// checks.cpp - recomputes the numbers quoted in the text of F10-06 (not a listing).
#include <cmath>
#include <cstdio>
#include <numbers>

int main()
{
    const double mass = 1.5, g = 9.81, rho = 1.225, S = 0.30, clMax = 1.2, cd0 = 0.03, k = 0.05;
    const double weight = mass * g, q1 = 0.5 * rho * S, deg = 180.0 / std::numbers::pi;
    std::printf("weight %.3f N, q1 = rho S / 2 = %.5f kg/m\n", weight, q1);

    // forensic: steepest level bank at 11 m/s before stall
    const double cl11 = weight / (q1 * 11.0 * 11.0);
    std::printf("CL level at 11 m/s %.4f; stall bank acos(CL/CLmax) = %.2f deg, n max %.3f\n", cl11,
                std::acos(cl11 / clMax) * deg, clMax / cl11);

    // worked example: tightest level turn at 12 m/s
    const double cl12 = weight / (q1 * 144.0);
    const double nMax = clMax / cl12, bankMax = std::acos(1.0 / nMax);
    std::printf("12 m/s: CL level %.4f, n max %.3f, bank max %.2f deg, radius %.2f m, "
                "turn rate %.1f deg/s\n",
                cl12, nMax, bankMax * deg, 144.0 / (g * std::tan(bankMax)),
                g * std::tan(bankMax) / 12.0 * deg);
    std::printf("12 m/s, 30 deg bank: turn rate %.1f deg/s, full circle %.1f s\n",
                g * std::tan(30.0 / deg) / 12.0 * deg,
                360.0 / (g * std::tan(30.0 / deg) / 12.0 * deg));

    // best L/D and glide
    const double clBest = std::sqrt(cd0 / k), ld = 1.0 / (2.0 * std::sqrt(cd0 * k));
    const double gam = std::atan(1.0 / ld);
    const double vg = std::sqrt(weight * std::cos(gam) / (q1 * clBest));
    std::printf("best CL %.4f, L/D %.3f, glide angle %.3f deg, speed %.3f m/s, sink %.3f m/s\n",
                clBest, ld, gam * deg, vg, vg * std::sin(gam));
    std::printf("glide distance from 100 m: %.0f m; time %.0f s\n", 100.0 * ld,
                100.0 / (vg * std::sin(gam)));
    std::printf("at best L/D, drag = weight / (L/D) = %.3f N (thrust needed in level flight)\n",
                weight / ld);
    std::printf("thrust ratio multirotor hover / aeroplane cruise = %.2f\n", ld);
    // minimum power: CL = sqrt(3 CD0 / k)
    std::printf("CL for minimum power sqrt(3 CD0/k) = %.4f (above CLmax %.1f)\n",
                std::sqrt(3.0 * cd0 / k), clMax);
    // drag split at best L/D: induced equals parasite
    std::printf("at best L/D: CD0 = %.3f, k CL^2 = %.3f\n", cd0, k * clBest * clBest);
    // stall speed scaling
    std::printf("stall speed at 60 deg bank: %.2f m/s (= %.2f x sqrt 2)\n",
                std::sqrt(weight / (q1 * clMax)) * std::sqrt(2.0), std::sqrt(weight / (q1 * clMax)));
    // check-yourself: double mass, stall speed factor
    std::printf("stall speed with 3.0 kg: %.2f m/s (factor %.4f)\n",
                std::sqrt(2.0 * weight / (q1 * clMax)), std::sqrt(2.0));
    return 0;
}
