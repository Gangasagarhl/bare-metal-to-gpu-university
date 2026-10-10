// usl.cpp - F12-15 Listing 2: fit the Universal Scalability Law to throughput measurements.
//   X(N) = lambda * N / (1 + sigma * (N - 1) + kappa * N * (N - 1))
// lambda is taken as X(1). Then  N * X(1) / X(N) - 1 = sigma * (N - 1) + kappa * N * (N - 1),
// which is linear in (sigma, kappa): an ordinary least-squares fit with two unknowns and no
// constant term. The predicted peak concurrency is N* = sqrt((1 - sigma) / kappa) when kappa > 0.
// Reads lines "N X" from standard input; lines starting with '#' are comments.
#include <cmath>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

int main()
{
    std::vector<double> n;
    std::vector<double> x;
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        std::istringstream in(line);
        double a = 0.0;
        double b = 0.0;
        if (in >> a >> b) {
            n.push_back(a);
            x.push_back(b);
        }
    }
    if (n.size() < 3 || n.front() != 1.0) {
        std::printf("need at least 3 points, the first at N = 1\n");
        return 1;
    }
    double const lambda = x.front();
    // Normal equations for y = sigma * u + kappa * v, with u = N - 1, v = N (N - 1).
    double suu = 0, suv = 0, svv = 0, suy = 0, svy = 0;
    for (std::size_t i = 0; i < n.size(); ++i) {
        double const u = n[i] - 1.0;
        double const v = n[i] * (n[i] - 1.0);
        double const y = n[i] * lambda / x[i] - 1.0;
        suu += u * u;
        suv += u * v;
        svv += v * v;
        suy += u * y;
        svy += v * y;
    }
    double const det = suu * svv - suv * suv;
    double const sigma = (suy * svv - svy * suv) / det;
    double const kappa = (svy * suu - suy * suv) / det;

    std::printf("lambda = X(1) = %.1f per s; fitted sigma = %.4f, kappa = %.5f\n", lambda, sigma,
                kappa);
    std::printf("%6s %12s %12s %9s %16s\n", "N", "measured X", "fitted X", "error",
                "linear N*X(1)");
    for (std::size_t i = 0; i < n.size(); ++i) {
        double const fit =
            lambda * n[i] / (1.0 + sigma * (n[i] - 1.0) + kappa * n[i] * (n[i] - 1.0));
        std::printf("%6.0f %12.1f %12.1f %8.1f%% %16.1f\n", n[i], x[i], fit,
                    100.0 * (x[i] - fit) / fit, lambda * n[i]);
    }
    if (kappa > 0.0 && sigma < 1.0) {
        double const peak_n = std::sqrt((1.0 - sigma) / kappa);
        double const peak_x =
            lambda * peak_n / (1.0 + sigma * (peak_n - 1.0) + kappa * peak_n * (peak_n - 1.0));
        std::printf("predicted peak: N* = %.1f, X(N*) = %.1f per s; beyond N* throughput falls\n",
                    peak_n, peak_x);
    } else {
        std::printf("kappa <= 0: no retrograde term in this data; the fit predicts no peak\n");
    }
    std::printf("contention limit 1/sigma: X can never exceed %.1f per s even with kappa = 0\n",
                sigma > 0.0 ? lambda / sigma : 0.0);
    return 0;
}
