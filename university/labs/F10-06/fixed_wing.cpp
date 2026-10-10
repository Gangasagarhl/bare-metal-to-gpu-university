// fixed_wing.cpp - lift, drag, stall and turns for a small pretend aeroplane (F10-06).
// Lift L = 1/2 rho V^2 S CL; drag D = 1/2 rho V^2 S CD with CD = CD0 + k CL^2.
// All values are exercise values (the air density is a round sea-level-like value).
#include <cmath>
#include <cstdio>
#include <initializer_list>
#include <numbers>

int main()
{
    const double mass = 1.5, g = 9.81, rho = 1.225, S = 0.30;  // kg, m/s^2, kg/m^3, m^2
    const double clMax = 1.2, cd0 = 0.03, k = 0.05;
    const double weight = mass * g;
    const double q1 = 0.5 * rho * S;  // lift = q1 V^2 CL

    std::printf("level flight (lift = weight): speed needed for each lift coefficient\n");
    std::printf("   CL  speed(m/s)  drag(N)  lift/drag  power to fly(W)\n");
    const double clBest = std::sqrt(cd0 / k);
    for (double cl : {0.2, 0.4, 0.6, clBest, 1.0, 1.2}) {
        const double v = std::sqrt(weight / (q1 * cl));
        const double cd = cd0 + k * cl * cl;
        const double drag = q1 * v * v * cd;
        std::printf("%5.3f %11.2f %8.3f %10.2f %16.2f\n", cl, v, drag, cl / cd, drag * v);
    }
    std::printf("best lift/drag at CL = sqrt(CD0/k) = %.4f: L/D = %.2f\n", clBest,
                1.0 / (2.0 * std::sqrt(cd0 * k)));
    const double vStall = std::sqrt(weight / (q1 * clMax));
    std::printf("stall speed in level flight: %.2f m/s\n", vStall);
    std::printf("for comparison, a multirotor of the same weight needs %.2f N of thrust to hover\n",
                weight);

    std::printf("\nlevel turns: load factor n = 1/cos(bank)\n");
    std::printf(" bank(deg)     n  stall speed(m/s)  radius at 12 m/s(m)\n");
    for (double b : {0.0, 15.0, 30.0, 45.0, 60.0, 70.0}) {
        const double rad = b * std::numbers::pi / 180.0;
        const double n = 1.0 / std::cos(rad);
        std::printf("%10.0f %5.2f %17.2f ", b, n, vStall * std::sqrt(n));
        if (b == 0.0) {
            std::printf("%20s\n", "straight line");
        } else {
            std::printf("%20.1f\n", 12.0 * 12.0 / (g * std::tan(rad)));
        }
    }
    return 0;
}
