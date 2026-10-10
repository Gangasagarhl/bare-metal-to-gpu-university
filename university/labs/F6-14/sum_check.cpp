// F6-14 Listing 4: the CPU side of the E3 acceptance test for 2^28 floats, without storing them.
// Same data as Listing 1: x_i = ((i * 2654435761) mod 2^32 mod 1000) / 1000.
// Compares: a plain float loop, a pairwise (tree) float sum like a GPU reduction, Kahan's
// compensated float sum, and a double sum, against an exact integer total.
#include <cmath>
#include <cstdint>
#include <cstdio>

int main()
{
    const std::uint32_t n = 1u << 28;
    float plain = 0.0f, kahan = 0.0f, comp = 0.0f;
    double dsum = 0.0;
    std::uint64_t exactThousandths = 0;               // the true sum is exactThousandths / 1000
    float stack[64];                                   // pairwise sum, streamed with a small stack
    std::uint32_t sizes[64];
    int top = 0;                                       // number of partial sums on the stack
    for (std::uint32_t i = 0; i < n; ++i) {
        std::uint32_t k = (i * 2654435761u) % 1000u;
        float x = static_cast<float>(k) / 1000.0f;
        exactThousandths += k;
        plain += x;
        float y = x - comp;
        float t = kahan + y;
        comp = (t - kahan) - y;
        kahan = t;
        dsum += x;
        stack[top] = x;
        sizes[top] = 1;
        ++top;
        while (top >= 2 && sizes[top - 1] == sizes[top - 2]) {   // merge two equal-sized partial sums
            stack[top - 2] += stack[top - 1];
            sizes[top - 2] += sizes[top - 1];
            --top;
        }
    }
    float pairwise = stack[0];                         // n is a power of two: one partial sum is left
    const double exact = static_cast<double>(exactThousandths) / 1000.0;
    std::printf("n = %u\n", n);
    std::printf("exact (integer total / 1000)   %.3f\n", exact);
    std::printf("%-30s %-18s %s\n", "method", "result", "relative error");
    auto row = [&](const char* name, double v) {
        std::printf("%-30s %-18.3f %.3e\n", name, v, std::fabs(v - exact) / exact);
    };
    row("float, one long loop", plain);
    row("float, pairwise (tree)", pairwise);
    row("float, Kahan compensated", kahan);
    row("double, one long loop", dsum);
    std::printf("float epsilon %.3e; log2(n) = %d\n", static_cast<double>(1.1920929e-7f), 28);
    return 0;
}
