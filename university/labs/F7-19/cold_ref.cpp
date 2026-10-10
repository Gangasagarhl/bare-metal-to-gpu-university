// F7-19 forensic evidence: Sam's benchmark report, produced by Sam's harness (this file).
// The "library" keeps a workspace that it creates on its first call, as many libraries do
// with handles, workspaces and kernel selection; Sam's harness times the two sides differently.
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <vector>

void mine(const std::vector<float>& A, const std::vector<float>& B, std::vector<float>& C, int n)
{
    std::fill(C.begin(), C.end(), 0.0f);
    for (int i = 0; i < n; ++i) {
        for (int k = 0; k < n; ++k) {
            const float a = A[i * n + k];
            for (int j = 0; j < n; ++j) {
                C[i * n + j] += a * B[k * n + j];
            }
        }
    }
}

void library(const std::vector<float>& A, const std::vector<float>& B, std::vector<float>& C, int n)
{
    static std::vector<float> workspace;               // created on the first call only
    if (workspace.empty()) {
        workspace.assign(std::size_t{16} << 20, 0.0f); // 64 MiB, zero-filled
    }
    mine(A, B, C, n);                                  // same arithmetic as "mine"
    workspace[0] += C[0];
}

template <class F>
double seconds(F f)
{
    const auto t0 = std::chrono::steady_clock::now();
    f();
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
}

int main()
{
    const int n = 128;
    std::vector<float> A(n * n, 0.5f), B(n * n, 0.25f), C(n * n);
    std::vector<double> t;
    for (int r = 0; r < 20; ++r) {
        t.push_back(seconds([&] { mine(A, B, C, n); }));
    }
    std::sort(t.begin(), t.end());
    const double tMine = (t[9] + t[10]) / 2.0;
    const double tLib = seconds([&] { library(A, B, C, n); });
    const double flop = 2.0 * n * n * n;
    std::printf("BENCHMARK REPORT (Sam's harness)\n");
    std::printf("size %d x %d x %d, FP32\n", n, n, n);
    std::printf("mine:    %.3f ms  %.3f GFLOP/s\n", tMine * 1e3, flop / tMine * 1e-9);
    std::printf("library: %.3f ms  %.3f GFLOP/s\n", tLib * 1e3, flop / tLib * 1e-9);
    std::printf("mine = %.0f %% of the library\n", 100.0 * tLib / tMine);
    return 0;
}
