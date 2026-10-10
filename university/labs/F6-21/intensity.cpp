// F6-21 Listing 4: arithmetic intensity of SGEMM, counted, not guessed.
// FLOPs: 2*M*N*K (one multiply and one add per term). Bytes: 4 per float.
// "requested" = bytes the threads ask for with load instructions (no cache counted);
// "compulsory" = every matrix read once and C written once (a perfect cache).
// A BM x BN tile per block reads a BM x K strip of A and a K x BN strip of B.
#include <cstdio>
#include <initializer_list>

int main()
{
    std::printf("%-6s %-26s %16s %16s %10s\n", "n", "scheme", "FLOP", "bytes", "FLOP/byte");
    for (double n : {1024.0, 4096.0}) {
        const double flop = 2.0 * n * n * n;
        const double naive = n * n * (2.0 * n * 4.0) + n * n * 4.0;    // per thread: K of A, K of B
        const double compulsory = 4.0 * (n * n + n * n + n * n);
        std::printf("%-6.0f %-26s %16.4g %16.4g %10.3f\n", n, "naive, requested", flop, naive,
                    flop / naive);
        const int tiles[][2] = {{16, 16}, {32, 32}, {64, 64}, {128, 64}, {128, 128}};
        for (const auto& t : tiles) {
            const double bm = t[0], bn = t[1];
            const double blocks = (n / bm) * (n / bn);
            const double bytes = blocks * (bm * n + n * bn) * 4.0 + n * n * 4.0;
            char name[40];
            std::snprintf(name, sizeof name, "block tile %3.0f x %-3.0f", bm, bn);
            std::printf("%-6.0f %-26s %16.4g %16.4g %10.3f\n", n, name, flop, bytes, flop / bytes);
        }
        std::printf("%-6.0f %-26s %16.4g %16.4g %10.3f\n", n, "compulsory (read once)", flop,
                    compulsory, flop / compulsory);
    }
    return 0;
}
