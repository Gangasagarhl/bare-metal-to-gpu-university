// F8-13 Listing 2: what tensor parallelism costs and saves for one MLP block (h -> 4h -> h),
// compared with data parallelism. Counts only; no timing. Input lines:
//   label h tokens N bytesPerValue
// where tokens = sequences x sequence length in one micro-batch on one GPU, N = GPUs in the group.
#include <cstdio>
#include <iostream>
#include <string>

int main()
{
    std::string label;
    double h, tokens, n, bytes;
    const double mib = 1024.0 * 1024.0;
    while (std::cin >> label >> h >> tokens >> n >> bytes) {
        const double weights = 2.0 * h * 4.0 * h;               // A (h x 4h) and B (4h x h), biases ignored
        const double flops = 2.0 * tokens * weights;            // forward: a multiply and an add per weight per token
        const double act = tokens * h * bytes;                  // one h-wide activation of the micro-batch
        const double ring = 2.0 * (n - 1) / n;                  // bytes sent per byte all-reduced (ring)
        std::printf("%s: h = %.0f, %.0f tokens per micro-batch, N = %.0f GPUs, %.0f bytes per value\n",
                    label.c_str(), h, tokens, n, bytes);
        std::printf("  MLP weights: %.0f values = %.2f MiB; per GPU with tensor parallelism: %.2f MiB\n",
                    weights, weights * bytes / mib, weights * bytes / n / mib);
        std::printf("  forward work per GPU: %.3g floating-point operations (1/N of %.3g)\n", flops / n, flops);
        std::printf("  tensor parallelism: 1 all-reduce of %.2f MiB in forward + 1 in backward per micro-batch;"
                    " each GPU sends %.2f MiB per micro-batch, on the critical path\n",
                    act / mib, 2.0 * ring * act / mib);
        std::printf("  data parallelism instead: 1 all-reduce of the MLP gradient, %.2f MiB, per step;"
                    " each GPU sends %.2f MiB per step, can overlap with backward\n\n",
                    weights * bytes / mib, ring * weights * bytes / mib);
    }
    return 0;
}
