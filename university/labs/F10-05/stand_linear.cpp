// stand_linear.cpp - forensic evidence for F10-05 ("The simulator says it hovers at half
// throttle; the real one does not"). A colleague fitted a straight line T = a w + b to the
// same thrust-stand table that fit_thrust.cpp makes, and used it in a simulator.
#include <cmath>
#include <cstdio>
#include <random>
#include <vector>

struct Sample
{
    double w;
    double thrust;
};

int main()
{
    std::mt19937 gen(2026);  // the same table as fit_thrust.cpp
    std::vector<Sample> data;
    for (double w = 100.0; w <= 900.0; w += 100.0) {
        const double error = (static_cast<double>(gen() % 2001) - 1000.0) / 1000.0 * 0.05;
        data.push_back({w, 1.0e-5 * w * w + error});
    }
    // ordinary least squares for a straight line
    const double n = static_cast<double>(data.size());
    double sw = 0, st = 0, sww = 0, swt = 0;
    for (const Sample& s : data) {
        sw += s.w;
        st += s.thrust;
        sww += s.w * s.w;
        swt += s.w * s.thrust;
    }
    const double a = (n * swt - sw * st) / (n * sww - sw * sw);
    const double b = (st - a * sw) / n;
    std::printf("colleague's model: T = %.6f * w + (%.4f)   [N, w in rad/s]\n", a, b);
    std::printf("speed(rad/s)  thrust(N)  line(N)  residual(N)\n");
    for (const Sample& s : data) {
        const double line = a * s.w + b;
        std::printf("%12.0f %10.3f %8.3f %12.3f\n", s.w, s.thrust, line, s.thrust - line);
    }
    const double hoverLine = (2.4525 - b) / a;
    std::printf("hover speed predicted by the line for 2.4525 N per rotor: %.1f rad/s\n",
                hoverLine);
    return 0;
}
