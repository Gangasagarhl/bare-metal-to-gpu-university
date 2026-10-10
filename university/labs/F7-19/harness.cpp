// F7-19 Listing 3: the measurement protocol of curriculum 13.4, shown on the CPU.
// Warm-up, then the median of 20 timed runs, for "my kernel" and for the reference,
// reported as GFLOP/s (2 * M * N * K floating-point operations) and as a percentage.
// Measured on the build container's CPU with sanitizers on: the numbers describe this run only.
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <functional>
#include <vector>

using Gemm = std::function<void(const std::vector<float>&, const std::vector<float>&, std::vector<float>&, int)>;

void naive(const std::vector<float>& A, const std::vector<float>& B, std::vector<float>& C, int n)
{
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            float acc = 0.0f;
            for (int k = 0; k < n; ++k) {
                acc += A[i * n + k] * B[k * n + j];   // B walked down a column: poor locality
            }
            C[i * n + j] = acc;
        }
    }
}

void reference(const std::vector<float>& A, const std::vector<float>& B, std::vector<float>& C, int n)
{
    std::fill(C.begin(), C.end(), 0.0f);
    for (int i = 0; i < n; ++i) {
        for (int k = 0; k < n; ++k) {
            const float a = A[i * n + k];
            for (int j = 0; j < n; ++j) {
                C[i * n + j] += a * B[k * n + j];    // rows of B and C: unit stride
            }
        }
    }
}

double medianSeconds(const Gemm& f, const std::vector<float>& A, const std::vector<float>& B,
                     std::vector<float>& C, int n, int warmup, int runs)
{
    for (int i = 0; i < warmup; ++i) {
        f(A, B, C, n);
    }
    std::vector<double> t;
    for (int i = 0; i < runs; ++i) {
        const auto t0 = std::chrono::steady_clock::now();
        f(A, B, C, n);
        const auto t1 = std::chrono::steady_clock::now();
        t.push_back(std::chrono::duration<double>(t1 - t0).count());
    }
    std::sort(t.begin(), t.end());
    return (t[runs / 2 - 1] + t[runs / 2]) / 2.0;     // median of an even count
}

int main()
{
    const int n = 256;
    std::vector<float> A(n * n), B(n * n), C1(n * n), C2(n * n);
    for (int i = 0; i < n * n; ++i) {
        A[i] = static_cast<float>(i % 13) / 8.0f;
        B[i] = static_cast<float>(i % 7) / 8.0f;
    }
    const double flop = 2.0 * n * n * n;
    const double tMine = medianSeconds(naive, A, B, C1, n, 2, 20);
    const double tRef = medianSeconds(reference, A, B, C2, n, 2, 20);
    int bad = 0;
    for (int i = 0; i < n * n; ++i) {
        bad += (C1[i] != C2[i]);                       // multiples of 1/64: both sums are exact
    }
    std::printf("n = %d, warm-up 2, median of 20 runs, %d mismatches\n", n, bad);
    std::printf("mine (naive)  %.3f ms  %.3f GFLOP/s\n", tMine * 1e3, flop / tMine * 1e-9);
    std::printf("reference     %.3f ms  %.3f GFLOP/s\n", tRef * 1e3, flop / tRef * 1e-9);
    std::printf("mine reaches %.1f %% of the reference on this machine, in this run\n", 100.0 * tRef / tMine);
    return bad == 0 ? 0 : 1;
}
