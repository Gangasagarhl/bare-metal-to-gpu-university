// F0-80 Listing 2: a dot product of length k = 4096 with reduced-precision inputs and two
// accumulators. The error is split into the part caused by rounding the inputs and the part caused
// by accumulating.
#include <cmath>
#include <cstdio>
#include <vector>

#include "lowp.hpp"

struct Case
{
    const Format* in;
    const Format* acc;
};

int main()
{
    const std::size_t k = 4096;
    std::uint64_t state = 80;
    std::vector<double> a(k), b(k);
    for (std::size_t i = 0; i < k; ++i) {
        a[i] = uniformMinus1To1(state);
        b[i] = uniformMinus1To1(state);
    }
    double exact = 0.0, absSum = 0.0; // double is far more precise than every format below
    for (std::size_t i = 0; i < k; ++i) {
        exact += a[i] * b[i];
        absSum += std::fabs(a[i] * b[i]);
    }
    std::printf("k = %zu, exact dot = %.10g, sum|a_i b_i| = %.10g\n", k, exact, absSum);
    std::printf("%-17s %-17s %-11s %-11s %-11s %-11s %-11s\n", "inputs", "accumulator", "total err",
                "input part", "acc. part", "input bnd", "acc. bnd");
    const Case cases[] = {{&kBinary32, &kBinary32}, {&kBinary16, &kBinary32},
                          {&kBinary16, &kBinary16}, {&kBfloat16, &kBinary32},
                          {&kBfloat16, &kBfloat16}, {&kE4M3, &kBinary32},
                          {&kE5M2, &kBinary32}};
    for (const Case& c : cases) {
        double roundedExact = 0.0, acc = 0.0;
        for (std::size_t i = 0; i < k; ++i) {
            const double ai = roundTo(a[i], *c.in), bi = roundTo(b[i], *c.in);
            roundedExact += ai * bi; // exact product of two short numbers, summed in double
            acc = roundTo(acc + roundTo(ai * bi, *c.acc),
                          *c.acc); // product and sum rounded to the accumulator
        }
        const double uIn = unitRoundoff(*c.in), uAcc = unitRoundoff(*c.acc);
        const double kd = static_cast<double>(k);
        std::printf("%-17s %-17s %-11.3e %-11.3e %-11.3e %-11.3e ", c.in->name, c.acc->name,
                    std::fabs(acc - exact), std::fabs(roundedExact - exact),
                    std::fabs(acc - roundedExact), (2.0 * uIn + uIn * uIn) * absSum);
        if (kd * uAcc < 1.0) {
            std::printf("%-11.3e\n",
                        kd * uAcc / (1.0 - kd * uAcc) * absSum); // gamma_k(u_acc) sum|a b|
        } else {
            std::printf("%-11s\n", "none (k u>=1)"); // the bound says nothing
        }
    }
    return 0;
}
