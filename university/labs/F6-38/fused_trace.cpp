// F6-38 Listing 3: a CPU model of the fused softmax kernel's per-row loop. The row is read in
// chunks of CHUNK elements, as one block reads tiles of a long row. After each chunk it logs
// the running maximum m and the running normaliser d. Mode "online" rescales d whenever m
// grows; mode "frozen" is the shipped bug: m is fixed by the first chunk and never updated,
// so the rescale step is skipped.
#include <cmath>
#include <cstdio>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

int main()
{
    std::string mode;
    std::size_t n = 0;
    std::size_t chunk = 0;
    float base = 0.0f;
    float rampPerChunk = 0.0f;   // input grows by this much from one chunk to the next
    if (!(std::cin >> mode >> n >> chunk >> base >> rampPerChunk) || chunk == 0 || n == 0) {
        std::printf("invalid input\n");
        return 1;
    }
    std::vector<float> x(n);
    for (std::size_t i = 0; i < n; ++i) {
        x[i] = base + rampPerChunk * static_cast<float>(i / chunk) + 0.01f * static_cast<float>(i % 5);
    }
    const bool frozen = (mode == "frozen");
    float m = -std::numeric_limits<float>::infinity();
    float d = 0.0f;
    std::printf("mode %s, n %zu, chunk %zu\n", mode.c_str(), n, chunk);
    std::printf("%5s %10s %10s %14s\n", "chunk", "chunk max", "m", "d");
    for (std::size_t c = 0; c * chunk < n; ++c) {
        float cmax = -std::numeric_limits<float>::infinity();
        for (std::size_t i = c * chunk; i < n && i < (c + 1) * chunk; ++i) {
            cmax = std::fmax(cmax, x[i]);
        }
        float mNew = std::fmax(m, cmax);
        if (frozen && c > 0) {
            mNew = m;                          // BUG: the normaliser is not moved to the new max
        }
        float s = 0.0f;
        for (std::size_t i = c * chunk; i < n && i < (c + 1) * chunk; ++i) {
            s += std::exp(x[i] - mNew);
        }
        const float scale = (c == 0) ? 0.0f : std::exp(m - mNew);
        d = d * scale + s;
        m = mNew;
        std::printf("%5zu %10.2f %10.2f %14.6g\n", c, static_cast<double>(cmax), static_cast<double>(m),
                    static_cast<double>(d));
    }
    int nans = 0;
    int infs = 0;
    float total = 0.0f;
    for (float v : x) {
        const float y = std::exp(v - m) / d;
        nans += std::isnan(y) ? 1 : 0;
        infs += std::isinf(y) ? 1 : 0;
        total += y;
    }
    std::printf("outputs: %d NaN, %d inf, sum of outputs %g (should be 1)\n", nans, infs,
                static_cast<double>(total));
    return 0;
}
