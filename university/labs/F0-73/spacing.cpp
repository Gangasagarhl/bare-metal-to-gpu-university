// F0-73 Listing 2: how far apart neighbouring floats and doubles are (the ULP), measured with
// nextafter.
#include <cmath>
#include <cstdio>
#include <limits>

template <typename T> void limits(const char* name)
{
    using L = std::numeric_limits<T>;
    std::printf("%-6s is_iec559=%d digits=%d epsilon=%.6g min normal=%.6g smallest subnormal=%.6g"
                " max=%.6g max_digits10=%d\n",
                name, static_cast<int>(L::is_iec559), L::digits, static_cast<double>(L::epsilon()),
                static_cast<double>(L::min()), static_cast<double>(L::denorm_min()),
                static_cast<double>(L::max()), L::max_digits10);
}

int main()
{
    limits<float>("float");
    limits<double>("double");
    std::printf("\n%-14s %-16s %-16s %s\n", "x", "ulp(x) float", "ulp(x) double", "ulp(x)/x float");
    const double xs[] = {1.0, 2.0, 3.0, 1000.0, 1.0e6, 16777216.0, 1.0e10, 1.0e-3};
    for (double x : xs) {
        const float xf = static_cast<float>(x);
        const float uf = std::nextafter(xf, std::numeric_limits<float>::infinity()) - xf;
        const double ud = std::nextafter(x, std::numeric_limits<double>::infinity()) - x;
        std::printf("%-14.6g %-16.6g %-16.6g %.3g\n", x, static_cast<double>(uf), ud,
                    static_cast<double>(uf) / static_cast<double>(xf));
    }
    // Integers are exact in float only up to 2^24.
    const float big = 16777216.0f;
    std::printf("\n2^24 + 1 in float = %.1f   2^24 + 2 in float = %.1f\n",
                static_cast<double>(big + 1.0f), static_cast<double>(big + 2.0f));
    // 0.1 is not exactly representable in either format.
    std::printf("0.1f = %.30f\n0.1  = %.30f\n", static_cast<double>(0.1f), 0.1);
    std::printf("0.1f + 0.2f == 0.3f ? %s     0.1 + 0.2 == 0.3 ? %s\n",
                (0.1f + 0.2f == 0.3f) ? "yes" : "no", (0.1 + 0.2 == 0.3) ? "yes" : "no");
    return 0;
}
