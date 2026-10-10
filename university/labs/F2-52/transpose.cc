// transpose.cc - out = transpose(in) for an n x n float matrix: naive versus blocked.
#include "bench.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <vector>

[[gnu::noinline]] void naive(std::vector<float> const& in, std::vector<float>& out, std::size_t n)
{
    for (std::size_t r = 0; r < n; ++r) {
        for (std::size_t c = 0; c < n; ++c) {
            out[c * n + r] = in[r * n + c];  // reads along a row, writes down a column
        }
    }
}

// Work on b x b tiles so that the lines of one tile of 'in' and of 'out' stay in cache.
[[gnu::noinline]] void blocked(std::vector<float> const& in, std::vector<float>& out, std::size_t n,
                               std::size_t b)
{
    for (std::size_t r0 = 0; r0 < n; r0 += b) {
        for (std::size_t c0 = 0; c0 < n; c0 += b) {
            std::size_t const rEnd = std::min(r0 + b, n);
            std::size_t const cEnd = std::min(c0 + b, n);
            for (std::size_t r = r0; r < rEnd; ++r) {
                for (std::size_t c = c0; c < cEnd; ++c) {
                    out[c * n + r] = in[r * n + c];
                }
            }
        }
    }
}

int main()
{
    std::size_t const n = 4096;
    std::vector<float> in(n * n);
    for (std::size_t i = 0; i < in.size(); ++i) {
        in[i] = static_cast<float>(i % 1000);
    }
    std::vector<float> ref(n * n), out(n * n);
    naive(in, ref, n);
    std::printf("n = %zu, median of 21 runs, every result checked against the naive one\n", n);
    auto const t0 = bench::run([&] { naive(in, out, n); bench::keep(out[1]); });
    std::printf("%-12s %9.2f ms\n", "naive", t0.median * 1e3);
    for (std::size_t b : {8, 16, 32, 64, 128}) {
        std::fill(out.begin(), out.end(), -1.0f);
        auto const t = bench::run([&] { blocked(in, out, n, b); bench::keep(out[1]); });
        bool const ok = (out == ref);
        std::printf("blocked b=%-3zu %9.2f ms  speed-up %.2f  %s\n", b, t.median * 1e3,
                    t0.median / t.median, ok ? "correct" : "WRONG");
    }
    return 0;
}
