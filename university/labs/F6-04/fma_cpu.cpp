// F6-04 Listing 3: two correct-looking CPU references for y = a*x + y disagree.
// "plain" rounds after the multiply and again after the add; std::fma rounds once.
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

static std::uint32_t bits(float f)
{
    std::uint32_t u = 0;
    std::memcpy(&u, &f, sizeof u);
    return u;
}

static float value(std::uint32_t i, std::uint32_t seed)   // same data as Listing 2
{
    std::uint32_t h = (i + seed) * 2654435761u;
    h ^= h >> 15;
    return static_cast<float>(h % 2000003u) / 1000.0f - 1000.0f;
}

int main()
{
    const int n = 1 << 20;
    const float a = 1.7f;
    int differ = 0;
    int first = -1;
    for (int i = 0; i < n; ++i) {
        const float x = value(i, 1);
        const float y = value(i, 2);
        const float product = a * x;          // rounded to float here
        const float plain = product + y;      // and rounded again here
        const float fused = std::fma(a, x, y);
        if (bits(plain) != bits(fused)) {
            ++differ;
            if (first < 0) { first = i; }
        }
    }
    std::printf("elements: %d, plain and fused results differ in: %d\n", n, differ);
    if (first >= 0) {
        const float x = value(first, 1);
        const float y = value(first, 2);
        const float product = a * x;
        const float plain = product + y;
        const float fused = std::fma(a, x, y);
        std::printf("first difference at i = %d: x = %.9g, y = %.9g\n", first, x, y);
        const double exact = static_cast<double>(a) * x + y;   // exact product fits in a double
        std::printf("  a = %.9g, a*x rounded to float = %.9g\n", a, product);
        std::printf("  exact a*x+y (double): %.9f\n", exact);
        std::printf("  plain: %.9g (bits 0x%08x)\n", plain, bits(plain));
        std::printf("  fused: %.9g (bits 0x%08x)\n", fused, bits(fused));
    }
    return 0;
}
