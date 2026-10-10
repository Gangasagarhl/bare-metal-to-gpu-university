// pfd_model.cpp - F11-20: average probability that a safety function is failed when it is
// demanded (PFDavg), for one channel (1oo1) and two channels where either can act (1oo2).
// Model: each channel suffers dangerous undetected failures at a constant rate lambda (per hour);
// a proof test every T hours finds and repairs them (perfect test, instant repair). A fraction
// beta of lambda is a common cause that fails both channels at once.
// The program prints the formulas derived in the chapter and a Monte Carlo estimate.
// Input lines: name arch lambda_per_hour T_hours beta intervals seed
// All rates are ILLUSTRATIVE numbers for the exercise, not data of any product.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <random>
#include <sstream>
#include <string>

struct Case {
    std::string name, arch;
    double lambda = 0.0, T = 0.0, beta = 0.0;
    long intervals = 0;
    unsigned long seed = 1;
};

double formula(const Case& c)
{
    if (c.arch == "1oo1") {
        return c.lambda * c.T / 2.0;
    }
    const double independent = (1.0 - c.beta) * c.lambda * c.T;
    return independent * independent / 3.0 + c.beta * c.lambda * c.T / 2.0;
}

double monte_carlo(const Case& c)
{
    std::mt19937_64 rng(c.seed);
    const bool two = (c.arch == "1oo2");
    const double rate_ind = two ? (1.0 - c.beta) * c.lambda : c.lambda;
    const double rate_cc = two ? c.beta * c.lambda : 0.0;
    std::exponential_distribution<double> ind(rate_ind);
    double down_hours = 0.0;
    for (long i = 0; i < c.intervals; ++i) {
        // time (from the last proof test) at which the function can no longer act
        double t_fail = two ? std::max(ind(rng), ind(rng)) : ind(rng);
        if (rate_cc > 0.0) {
            std::exponential_distribution<double> cc(rate_cc);
            t_fail = std::min(t_fail, cc(rng));
        }
        down_hours += std::max(0.0, c.T - t_fail);   // failed, unnoticed until the next test
    }
    return down_hours / (static_cast<double>(c.intervals) * c.T);
}

int main()
{
    std::printf("%-26s %-4s %9s %7s %5s | %11s %11s %7s\n", "case", "arch", "lambda/h", "T (h)",
                "beta", "formula", "simulated", "ratio");
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        std::istringstream in(line);
        Case c;
        if (!(in >> c.name >> c.arch >> c.lambda >> c.T >> c.beta >> c.intervals >> c.seed)) {
            std::cerr << "bad line: " << line << '\n';
            return 2;
        }
        const double f = formula(c);
        const double s = monte_carlo(c);
        std::printf("%-26s %-4s %9.2g %7.0f %5.2f | %11.3e %11.3e %7.3f\n", c.name.c_str(),
                    c.arch.c_str(), c.lambda, c.T, c.beta, f, s, s / f);
    }
    std::puts("formula: 1oo1 lambda*T/2; 1oo2 ((1-beta)*lambda*T)^2/3 + beta*lambda*T/2");
    return 0;
}
