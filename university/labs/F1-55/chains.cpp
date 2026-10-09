// F1-55 Listing 1: latency versus throughput on the CPU you are using.
// The same number of multiply-add steps, done as one dependent chain
// and as four independent chains. Times are measurements of this machine only.
#include <chrono>
#include <cstdio>

// O2 for these two functions only, so the loops are real machine loops
// (the lab flags build everything else without optimisation).
[[gnu::optimize("O2"), gnu::noinline]] double oneChain(double x, long steps)
{
    for (long i = 0; i < steps; ++i) {
        x = x * 0.999999 + 0.000001;   // each step needs the previous result
    }
    return x;
}

[[gnu::optimize("O2"), gnu::noinline]] double fourChains(double x, long steps)
{
    double a = x, b = x + 1, c = x + 2, d = x + 3;
    for (long i = 0; i < steps / 4; ++i) {
        a = a * 0.999999 + 0.000001;   // the four lines do not depend
        b = b * 0.999999 + 0.000001;   // on each other, so the core can
        c = c * 0.999999 + 0.000001;   // work on them at the same time
        d = d * 0.999999 + 0.000001;
    }
    return a + b + c + d;
}

double nsPerStep(std::chrono::steady_clock::duration d, long steps)
{
    return std::chrono::duration<double, std::nano>(d).count() / static_cast<double>(steps);
}

int main()
{
    const long steps = 80'000'000;
    std::printf("steps per run: %ld (same total work in both versions)\n", steps);
    std::printf("%-6s %-22s %-22s %s\n", "run", "one chain [ns/step]", "four chains [ns/step]", "ratio");
    for (int run = 1; run <= 5; ++run) {
        auto t0 = std::chrono::steady_clock::now();
        double r1 = oneChain(2.0, steps);
        auto t1 = std::chrono::steady_clock::now();
        double r4 = fourChains(2.0, steps);
        auto t2 = std::chrono::steady_clock::now();
        double one = nsPerStep(t1 - t0, steps);
        double four = nsPerStep(t2 - t1, steps);
        std::printf("%-6d %-22.3f %-22.3f %.2f   (results %.4f %.4f)\n", run, one, four, one / four, r1, r4);
    }
    return 0;
}
