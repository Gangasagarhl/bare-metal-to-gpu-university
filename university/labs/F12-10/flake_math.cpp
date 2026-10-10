// flake_math.cpp - how many runs it takes to see a flaky test fail, or to trust a fix.
// If one run fails with probability p, independently, then n runs all pass with
// probability (1 - p)^n. To be confident c that a still-broken test would have failed at least
// once, you need (1 - p)^n <= 1 - c, so n >= ln(1 - c) / ln(1 - p).
#include <cmath>
#include <cstdio>

int main()
{
    const double ps[] = {0.5, 0.1, 0.01, 0.001};
    std::printf("failure rate p   P(10 runs all pass)   runs for 95 %%   runs for 99 %%\n");
    for (double p : ps) {
        const double all_pass_10 = std::pow(1.0 - p, 10);
        const double n95 = std::ceil(std::log(1.0 - 0.95) / std::log(1.0 - p));
        const double n99 = std::ceil(std::log(1.0 - 0.99) / std::log(1.0 - p));
        std::printf("%13.3f   %19.4f   %13.0f   %13.0f\n", p, all_pass_10, n95, n99);
    }
    return 0;
}
