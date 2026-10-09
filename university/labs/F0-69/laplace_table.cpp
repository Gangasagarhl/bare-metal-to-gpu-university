// F0-69 Listing 1: checks four rows of the Laplace transform table by computing
//   F(s) = integral from 0 to infinity of f(t) e^(-s t) dt
// numerically (trapezoid rule up to t = 60 s, where e^(-s t) is negligible for s >= 1).
#include <cmath>
#include <format>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

double laplaceNumeric(const std::function<double(double)>& f, double s)
{
    const double tEnd = 60.0;
    const int n = 600000;
    const double h = tEnd / n;
    double sum = 0.5 * (f(0.0) + f(tEnd) * std::exp(-s * tEnd));
    for (int k = 1; k < n; ++k) {
        const double t = k * h;
        sum += f(t) * std::exp(-s * t);
    }
    return sum * h;
}

struct Row
{
    std::string name;
    std::function<double(double)> f;
    std::function<double(double)> table;
};

int main()
{
    const std::vector<Row> rows = {
        {"1 (unit step)   -> 1/s", [](double) { return 1.0; }, [](double s) { return 1.0 / s; }},
        {"t (ramp)        -> 1/s^2", [](double t) { return t; }, [](double s) { return 1.0 / (s * s); }},
        {"e^(-2t)         -> 1/(s+2)", [](double t) { return std::exp(-2.0 * t); },
         [](double s) { return 1.0 / (s + 2.0); }},
        {"sin(3t)         -> 3/(s^2+9)", [](double t) { return std::sin(3.0 * t); },
         [](double s) { return 3.0 / (s * s + 9.0); }},
    };
    int failures = 0;
    for (const Row& r : rows) {
        for (double s : {1.0, 2.0, 5.0}) {
            const double num = laplaceNumeric(r.f, s);
            const double tab = r.table(s);
            const bool ok = std::abs(num - tab) < 1e-6;
            std::cout << std::format("{:<30} s = {}: numeric {:.7f}  table {:.7f}  {}\n", r.name, s, num, tab,
                                     ok ? "PASS" : "FAIL");
            failures += ok ? 0 : 1;
        }
    }
    return failures == 0 ? 0 : 1;
}
