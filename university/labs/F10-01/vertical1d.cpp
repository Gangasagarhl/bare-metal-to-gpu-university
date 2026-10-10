// vertical1d.cpp - the first simulator of DN201: a multirotor that can only move up and down.
// m dv/dt = T - m g - k v|v|  (thrust up, weight down, drag against the motion), z >= 0.
// Exercise values: mass 1.0 kg, drag coefficient k = 0.05 N per (m/s)^2; steps of 1 ms.
#include <cmath>
#include <cstdio>

int main()
{
    const double mass = 1.0, g = 9.81, k = 0.05, dt = 0.001;
    const double weight = mass * g;
    double z = 0.0, v = 0.0;  // height (m) and vertical speed (m/s), up is positive
    std::printf("  t(s)  thrust/weight  z(m)    v(m/s)  a(m/s^2)\n");
    for (int step = 0; step <= 10000; ++step) {
        const double t = step * dt;
        double ratio = 0.9;                  // 0 s to 1 s: thrust below weight
        if (step >= 1000) ratio = 1.0;       // 1 s to 2 s: thrust equals weight
        if (step >= 2000) ratio = 1.2;       // 2 s to 8 s: climb
        if (step >= 8000) ratio = 0.8;       // 8 s to 10 s: thrust below weight again
        const double thrust = ratio * weight;
        double a = (thrust - weight - k * v * std::fabs(v)) / mass;
        if (z <= 0.0 && a < 0.0 && v <= 0.0) {
            a = 0.0;                         // the ground pushes back: no falling through it
            v = 0.0;
        }
        if (step % 500 == 0) {
            std::printf("%6.1f %14.2f %6.2f %8.3f %9.3f\n", t, ratio, z, v, a);
        }
        v += a * dt;                         // semi-implicit Euler (MA302 F0-76)
        z += v * dt;
    }
    std::printf("terminal climb speed for thrust/weight 1.2: sqrt(0.2 m g / k) = %.3f m/s\n",
                std::sqrt(0.2 * weight / k));
    return 0;
}
