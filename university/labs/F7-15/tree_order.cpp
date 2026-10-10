// F7-15 Listing 3: the same float sum in different orders.
// Replays, on the CPU, the order in which a GPU reduction adds numbers:
// a grid-stride loop per thread, then a shuffle tree inside each wave of width W,
// then the wave totals of a block, then the block totals.
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <random>
#include <vector>

float gpuOrderSum(const std::vector<float>& x, int blocks, int threads, int width)
{
    const std::size_t total = static_cast<std::size_t>(blocks) * threads;
    std::vector<float> blockTotal(blocks, 0.0f);
    std::vector<float> lane(threads);
    for (int b = 0; b < blocks; ++b) {
        for (int t = 0; t < threads; ++t) {            // grid-stride loop of one thread
            float acc = 0.0f;
            for (std::size_t i = b * static_cast<std::size_t>(threads) + t; i < x.size(); i += total) {
                acc += x[i];
            }
            lane[t] = acc;
        }
        std::vector<float> waveTotal;
        for (int w0 = 0; w0 < threads; w0 += width) {  // shuffle-down tree inside one wave
            for (int d = width / 2; d >= 1; d /= 2) {
                for (int l = 0; l < d; ++l) {
                    lane[w0 + l] += lane[w0 + l + d];
                }
            }
            waveTotal.push_back(lane[w0]);
        }
        float acc = 0.0f;                              // wave 0 adds the wave totals in order
        for (float v : waveTotal) {
            acc += v;
        }
        blockTotal[b] = acc;
    }
    float sum = 0.0f;                                  // a second pass adds the block totals
    for (float v : blockTotal) {
        sum += v;
    }
    return sum;
}

int main()
{
    const std::size_t n = std::size_t{1} << 24;
    std::mt19937 gen(2026);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    std::vector<float> x(n);
    for (auto& v : x) {
        v = dist(gen);
    }
    double ref = 0.0;                                  // double-precision Kahan sum as reference
    double c = 0.0;
    double absSum = 0.0;
    for (float v : x) {
        double y = static_cast<double>(v) - c;
        double t = ref + y;
        c = (t - ref) - y;
        ref = t;
        absSum += std::fabs(static_cast<double>(v));
    }
    float seq = 0.0f;
    for (float v : x) {
        seq += v;
    }
    struct Shape { const char* name; int blocks; int threads; int width; };
    const Shape shapes[] = {
        {"W=32, 1024 blocks x 256", 1024, 256, 32},
        {"W=64, 1024 blocks x 256", 1024, 256, 64},
        {"W=64,  440 blocks x 256", 440, 256, 64},
        {"W=64, 1024 blocks x 512", 1024, 512, 64},
    };
    std::printf("n = %zu floats, uniform in [-1,1), seed 2026\n", n);
    std::printf("reference (double, Kahan)   %.6f\n", ref);
    // First-order bound for a sum whose longest chain of additions has depth d:
    // |error| <= d * u * sum|x|, with u = 2^-24 for float (round to nearest).
    const double u = std::ldexp(1.0, -24);
    std::printf("sum of |x|                  %.3f\n", absSum);
    std::printf("%-27s %-15s %-12s %-6s %s\n", "order", "result", "error", "depth", "bound");
    std::printf("%-27s %-15.6f %+-12.6f %-6zu %.3f\n", "sequential float", static_cast<double>(seq),
                static_cast<double>(seq) - ref, n, static_cast<double>(n) * u * absSum);
    for (const Shape& s : shapes) {
        float r = gpuOrderSum(x, s.blocks, s.threads, s.width);
        const std::size_t per = (n + static_cast<std::size_t>(s.blocks) * s.threads - 1) /
                                (static_cast<std::size_t>(s.blocks) * s.threads);
        const std::size_t depth = per + static_cast<std::size_t>(std::log2(s.width)) +
                                  static_cast<std::size_t>(s.threads / s.width) + s.blocks;
        std::printf("%-27s %-15.6f %+-12.6f %-6zu %.3f\n", s.name, static_cast<double>(r),
                    static_cast<double>(r) - ref, depth, static_cast<double>(depth) * u * absSum);
    }

    std::int64_t iseq = 0;                             // integers: every order gives the same sum
    std::int64_t irev = 0;
    for (std::size_t i = 0; i < n; ++i) {
        iseq += static_cast<std::int64_t>(i % 1000);
        irev += static_cast<std::int64_t>((n - 1 - i) % 1000);
    }
    std::printf("integer sums, forward and backward: %lld %lld\n", static_cast<long long>(iseq),
                static_cast<long long>(irev));
    return 0;
}
