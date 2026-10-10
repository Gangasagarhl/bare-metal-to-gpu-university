// F6-38 worked example: the online normaliser step by step on x = {1, 3, 2}, then the same
// result obtained by merging two partial states, as two warps (or two lanes) would.
#include <cmath>
#include <cstdio>
#include <limits>
#include <vector>

struct State
{
    double m;   // running maximum
    double d;   // running normaliser: sum of exp(x - m) so far
};

State merge(State a, State b)
{
    const double m = std::fmax(a.m, b.m);
    return {m, a.d * std::exp(a.m - m) + b.d * std::exp(b.m - m)};
}

int main()
{
    const std::vector<double> x = {1.0, 3.0, 2.0};
    State s = {-std::numeric_limits<double>::infinity(), 0.0};
    for (double v : x) {
        const double mNew = std::fmax(s.m, v);
        s.d = s.d * std::exp(s.m - mNew) + std::exp(v - mNew);
        s.m = mNew;
        std::printf("after x = %.0f: m = %.0f, d = %.6f\n", v, s.m, s.d);
    }
    std::printf("softmax:");
    for (double v : x) {
        std::printf(" %.6f", std::exp(v - s.m) / s.d);
    }
    const State left = {1.0, 1.0};                          // state of {1}
    const State right = {3.0, 1.0 + std::exp(2.0 - 3.0)};   // state of {3, 2}
    const State both = merge(left, right);
    std::printf("\nmerge({m=1, d=1}, {m=3, d=%.6f}) = {m=%.0f, d=%.6f}\n", right.d, both.m, both.d);
    return 0;
}
