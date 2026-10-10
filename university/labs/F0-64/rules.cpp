// F0-64 Listing 2: checks the derivative rules numerically. For each function the
// rule's answer is compared with a central difference (f(t+h) - f(t-h)) / (2h).
#include <cmath>
#include <format>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

struct Case
{
    std::string name;
    std::function<double(double)> f;
    std::function<double(double)> rule;   // the derivative the rule predicts
};

int main()
{
    const std::vector<Case> cases = {
        {"power:    t^3            -> 3 t^2", [](double t) { return t * t * t; },
         [](double t) { return 3.0 * t * t; }},
        {"sum:      t^2 + 5 t      -> 2 t + 5", [](double t) { return t * t + 5.0 * t; },
         [](double t) { return 2.0 * t + 5.0; }},
        {"product:  t^2 sin t      -> 2t sin t + t^2 cos t", [](double t) { return t * t * std::sin(t); },
         [](double t) { return 2.0 * t * std::sin(t) + t * t * std::cos(t); }},
        {"chain:    sin(3 t)       -> 3 cos(3 t)", [](double t) { return std::sin(3.0 * t); },
         [](double t) { return 3.0 * std::cos(3.0 * t); }},
        {"exp:      e^(-2 t)       -> -2 e^(-2 t)", [](double t) { return std::exp(-2.0 * t); },
         [](double t) { return -2.0 * std::exp(-2.0 * t); }},
        {"quotient: 1 / (1 + t)    -> -1 / (1 + t)^2", [](double t) { return 1.0 / (1.0 + t); },
         [](double t) { return -1.0 / ((1.0 + t) * (1.0 + t)); }},
    };
    const double t = 2.0;
    const double h = 1e-5;
    int failures = 0;
    for (const Case& c : cases) {
        const double numeric = (c.f(t + h) - c.f(t - h)) / (2.0 * h);
        const double predicted = c.rule(t);
        const bool ok = std::abs(numeric - predicted) < 1e-6 * (1.0 + std::abs(predicted));
        std::cout << std::format("{:<48} at t = 2: rule {:>12.8f}, numeric {:>12.8f}  {}\n", c.name, predicted,
                                 numeric, ok ? "PASS" : "FAIL");
        if (!ok) {
            ++failures;
        }
    }
    std::cout << (failures == 0 ? "all rules agree with the numbers\n" : "some rules disagree\n");
    return failures == 0 ? 0 : 1;
}
