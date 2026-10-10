// F1-61 Listing 3: why matrix units multiply in a short format but accumulate in a long one.
// A software model of rounding to a format with `bits` significant bits
// (11 for IEEE 754 binary16 "FP16", 8 for bfloat16, 24 for binary32 "FP32"),
// round-to-nearest-even, ignoring overflow and subnormals (our values stay in range).
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <vector>

double roundTo(double x, int bits)
{
    if (x == 0.0) {
        return 0.0;
    }
    int e = 0;
    std::frexp(x, &e);                           // x = m * 2^e with 0.5 <= |m| < 1
    double scale = std::ldexp(1.0, bits - e);    // move the kept bits left of the point
    return std::nearbyint(x * scale) / scale;    // default rounding mode: to nearest, ties to even
}

// Dot product: inputs rounded to `inBits`, each product rounded to `accBits`,
// running sum rounded to `accBits` after every addition.
double dot(const std::vector<double>& a, const std::vector<double>& b, int inBits, int accBits)
{
    double sum = 0.0;
    for (std::size_t i = 0; i < a.size(); ++i) {
        double p = roundTo(roundTo(a[i], inBits) * roundTo(b[i], inBits), accBits);
        sum = roundTo(sum + p, accBits);
    }
    return sum;
}

int main()
{
    const std::size_t k = 4096;                  // length of one dot product (the K of a GEMM)
    std::vector<double> ones(k, 1.0);
    std::printf("sum of %zu ones:  FP16 accumulate %.1f   FP32 accumulate %.1f\n", k,
                dot(ones, ones, 11, 11), dot(ones, ones, 11, 24));

    std::vector<double> a(k), b(k);
    std::uint32_t s = 12345;                     // small deterministic pseudo-random sequence
    for (std::size_t i = 0; i < k; ++i) {
        s = s * 1664525u + 1013904223u; a[i] = static_cast<double>(s >> 8) / 16777216.0;   // [0, 1)
        s = s * 1664525u + 1013904223u; b[i] = static_cast<double>(s >> 8) / 16777216.0;
    }
    double exact = 0.0;
    for (std::size_t i = 0; i < k; ++i) { exact += roundTo(a[i], 11) * roundTo(b[i], 11); }
    struct Case { const char* name; int in; int acc; };
    for (Case c : {Case{"FP16 inputs, FP16 accumulate", 11, 11}, Case{"FP16 inputs, FP32 accumulate", 11, 24},
                   Case{"BF16 inputs, FP32 accumulate", 8, 24}}) {
        double r = dot(a, b, c.in, c.acc);
        std::printf("%-30s result %10.4f  relative error vs exact sum of FP16 products %.2e\n", c.name, r,
                    std::fabs(r - exact) / exact);
    }
    std::printf("exact (double) sum of the FP16-rounded products: %.4f\n", exact);
    return 0;
}
