// glide.cpp - the pretend aeroplane of fixed_wing.cpp glides with its motor off at the
// lift coefficient of best lift/drag. Point-mass model in the vertical plane:
//   m dV/dt = -D - m g sin(gamma),   m V dgamma/dt = L - m g cos(gamma)
// gamma is the flight-path angle (negative = descending). Semi-implicit Euler, 1 ms steps.
#include <cmath>
#include <cstdio>
#include <numbers>

int main()
{
    const double mass = 1.5, g = 9.81, rho = 1.225, S = 0.30, cd0 = 0.03, k = 0.05;
    const double cl = std::sqrt(cd0 / k), cd = cd0 + k * cl * cl;
    const double q1 = 0.5 * rho * S, dt = 0.001, deg = 180.0 / std::numbers::pi;
    double v = 15.0, gamma = 0.0, x = 0.0, z = 100.0;  // m/s, rad, m, m
    std::printf("  t(s)   speed(m/s)  path angle(deg)  distance(m)  height(m)\n");
    for (int step = 0; step <= 60000; ++step) {
        if (step % 4000 == 0) {
            std::printf("%6.0f %12.3f %16.3f %12.1f %10.2f\n", step * dt, v, gamma * deg, x, z);
        }
        const double lift = q1 * v * v * cl, drag = q1 * v * v * cd;
        v += dt * (-drag - mass * g * std::sin(gamma)) / mass;
        gamma += dt * (lift - mass * g * std::cos(gamma)) / (mass * v);
        x += dt * v * std::cos(gamma);
        z += dt * v * std::sin(gamma);
    }
    const double gammaGlide = -std::atan(cd / cl);
    std::printf("steady glide predicted: angle %.3f deg, speed %.3f m/s, glide ratio %.2f\n",
                gammaGlide * deg, std::sqrt(mass * g * std::cos(gammaGlide) / (q1 * cl)), cl / cd);
    return 0;
}
