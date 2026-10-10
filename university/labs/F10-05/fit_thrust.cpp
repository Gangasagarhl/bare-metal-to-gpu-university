// fit_thrust.cpp - estimating kT from a thrust-stand table (F10-05).
// The "measurements" are made by this program from the course model T = kT w^2 with
// kT = 1.0e-5 N per (rad/s)^2, plus a small pseudo-random error of at most 0.05 N, so that
// you can see how well a fit recovers a value you know. A real stand gives real data.
#include <cmath>
#include <cstdio>
#include <random>
#include <vector>

struct Sample
{
    double w;       // rad/s
    double thrust;  // N
};

std::vector<Sample> standData()
{
    std::mt19937 gen(2026);  // fixed seed: the same table on every run
    std::vector<Sample> data;
    for (double w = 100.0; w <= 900.0; w += 100.0) {
        const double error = (static_cast<double>(gen() % 2001) - 1000.0) / 1000.0 * 0.05;
        data.push_back({w, 1.0e-5 * w * w + error});
    }
    return data;
}

int main()
{
    const std::vector<Sample> data = standData();
    // least squares for T = k w^2: minimise sum (T - k w^2)^2  =>  k = sum(T w^2) / sum(w^4)
    double num = 0.0, den = 0.0;
    for (const Sample& s : data) {
        num += s.thrust * s.w * s.w;
        den += s.w * s.w * s.w * s.w;
    }
    const double k = num / den;
    std::printf("speed(rad/s)  thrust(N)  model k w^2(N)  residual(N)\n");
    for (const Sample& s : data) {
        const double model = k * s.w * s.w;
        std::printf("%12.0f %10.3f %15.3f %12.3f\n", s.w, s.thrust, model, s.thrust - model);
    }
    std::printf("fitted kT = %.4e N per (rad/s)^2 (value used to make the data: 1.0000e-05)\n", k);
    std::printf("hover speed from the fit for 2.4525 N per rotor: %.1f rad/s\n",
                std::sqrt(2.4525 / k));
    return 0;
}
