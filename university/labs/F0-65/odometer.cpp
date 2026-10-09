// F0-65 Listing 2: an odometer and an energy meter. It reads (time, speed, power)
// samples from standard input and accumulates distance = integral of speed and
// energy = integral of power with the trapezoid rule.
#include <format>
#include <iostream>

int main()
{
    double tPrev = 0.0;
    double vPrev = 0.0;
    double pPrev = 0.0;
    double distance = 0.0;
    double energy = 0.0;
    bool first = true;
    double t = 0.0;
    double v = 0.0;
    double p = 0.0;
    std::cout << " t (s)  speed (m/s)  power (W)  distance (m)  energy (J)\n";
    while (std::cin >> t >> v >> p) {
        if (!first) {
            const double dt = t - tPrev;
            distance += 0.5 * (v + vPrev) * dt;
            energy += 0.5 * (p + pPrev) * dt;
        }
        first = false;
        std::cout << std::format("{:>6.1f} {:>12.2f} {:>10.2f} {:>13.3f} {:>11.3f}\n", t, v, p, distance, energy);
        tPrev = t;
        vPrev = v;
        pPrev = p;
    }
    std::cout << std::format("total: {:.3f} m, {:.3f} J\n", distance, energy);
    return 0;
}
