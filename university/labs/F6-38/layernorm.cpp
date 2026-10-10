// F6-38 Listing 2: the variance inside layer norm, computed three ways in float and compared with
// double. one-pass: E[x^2] - E[x]^2 from two running sums (one read, but cancels badly).
// two-pass: mean first, then the mean of squared differences (two reads).
// Welford: one read, with a running mean and M2 that are merged like the online softmax state.
#include <cmath>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

struct Welford
{
    float mean = 0.0f;
    float m2 = 0.0f;   // sum of squared differences from the current mean
    float count = 0.0f;
};

void push(Welford& w, float x)
{
    w.count += 1.0f;
    const float delta = x - w.mean;
    w.mean += delta / w.count;
    w.m2 += delta * (x - w.mean);
}

// merging two partial states: what two warps (or two threads) do in the fused kernel
Welford merge(const Welford& a, const Welford& b)
{
    Welford r;
    r.count = a.count + b.count;
    if (r.count == 0.0f) {
        return r;
    }
    const float delta = b.mean - a.mean;
    r.mean = a.mean + delta * (b.count / r.count);
    r.m2 = a.m2 + b.m2 + delta * delta * (a.count * b.count / r.count);
    return r;
}

int main()
{
    std::string name;
    std::size_t n = 0;
    float offset = 0.0f;
    float spread = 0.0f;
    std::printf("%-10s %6s %9s %12s | %12s %12s %12s %12s\n", "case", "n", "offset", "true var",
                "one-pass", "two-pass", "Welford", "Welford x2");
    while (std::cin >> name >> n >> offset >> spread) {
        std::vector<float> x(n);
        for (std::size_t i = 0; i < n; ++i) {
            x[i] = offset + spread * static_cast<float>(static_cast<int>(i % 11) - 5);
        }
        double dm = 0.0;
        for (float v : x) {
            dm += v;
        }
        dm /= static_cast<double>(n);
        double dv = 0.0;
        for (float v : x) {
            dv += (v - dm) * (v - dm);
        }
        dv /= static_cast<double>(n);

        float s = 0.0f;
        float s2 = 0.0f;
        for (float v : x) {
            s += v;
            s2 += v * v;
        }
        const float fn = static_cast<float>(n);
        const float onePass = s2 / fn - (s / fn) * (s / fn);

        float mean = 0.0f;
        for (float v : x) {
            mean += v;
        }
        mean /= fn;
        float var2 = 0.0f;
        for (float v : x) {
            var2 += (v - mean) * (v - mean);
        }
        var2 /= fn;

        Welford w;
        Welford lo;
        Welford hi;
        for (std::size_t i = 0; i < n; ++i) {
            push(w, x[i]);
            push(i < n / 2 ? lo : hi, x[i]);
        }
        const Welford both = merge(lo, hi);
        std::printf("%-10s %6zu %9.0f %12.6g | %12.6g %12.6g %12.6g %12.6g\n", name.c_str(), n,
                    static_cast<double>(offset), dv, static_cast<double>(onePass),
                    static_cast<double>(var2), static_cast<double>(w.m2 / w.count),
                    static_cast<double>(both.m2 / both.count));
    }
    return 0;
}
