// availability.cpp - DS403 F5-44 worked example: how likely is "the whole cluster" usable?
// Each node is up with probability p, independently (a model, not a measurement).
//   all-nodes design: usable only if every node is up          -> p^n
//   majority design:  usable if more than half of the nodes are up
#include <cmath>
#include <cstdio>

namespace {
double choose(int n, int k)
{
    double c = 1.0;
    for (int i = 1; i <= k; ++i) c = c * (n - k + i) / i;
    return c;
}

double at_least(int n, int k, double p)
{
    double sum = 0.0;
    for (int up = k; up <= n; ++up) sum += choose(n, up) * std::pow(p, up) * std::pow(1.0 - p, n - up);
    return sum;
}
}  // namespace

int main()
{
    const double ps[] = {0.99, 0.999};
    for (double p : ps) {
        std::printf("node up with probability p = %.3f\n", p);
        std::printf("  n   all n up (p^n)   majority up   down: all-design   down: majority\n");
        for (int n = 1; n <= 7; n += 2) {
            double all = std::pow(p, n);
            double maj = at_least(n, n / 2 + 1, p);
            std::printf("  %d   %.9f      %.9f   %.3e          %.3e\n", n, all, maj, 1.0 - all, 1.0 - maj);
        }
    }
    return 0;
}
