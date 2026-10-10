// HW203 practical (P), the "then measure" half: the four loop nests of cache_exam.cpp on real
// hardware, n x n doubles with n = 4096 (128 MiB per matrix). Times are measurements on the
// machine named in the .log, not specifications (AH-23). Built with -O2 and no sanitizers.
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <vector>

using Clock = std::chrono::steady_clock;

double ms(Clock::time_point t0, Clock::time_point t1)
{
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

double sumRows(const std::vector<double>& a, std::size_t n)
{
    double s = 0.0;
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = 0; j < n; ++j) {
            s += a[i * n + j];
        }
    }
    return s;
}

double sumCols(const std::vector<double>& a, std::size_t n)
{
    double s = 0.0;
    for (std::size_t j = 0; j < n; ++j) {
        for (std::size_t i = 0; i < n; ++i) {
            s += a[i * n + j];
        }
    }
    return s;
}

void transposeNaive(const std::vector<double>& a, std::vector<double>& b, std::size_t n)
{
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = 0; j < n; ++j) {
            b[j * n + i] = a[i * n + j];
        }
    }
}

void transposeBlocked(const std::vector<double>& a, std::vector<double>& b, std::size_t n,
                      std::size_t tile)
{
    for (std::size_t ii = 0; ii < n; ii += tile) {
        for (std::size_t jj = 0; jj < n; jj += tile) {
            for (std::size_t i = ii; i < std::min(ii + tile, n); ++i) {
                for (std::size_t j = jj; j < std::min(jj + tile, n); ++j) {
                    b[j * n + i] = a[i * n + j];
                }
            }
        }
    }
}

int main()
{
    const std::size_t n = 4096;
    std::vector<double> a(n * n);
    std::vector<double> b(n * n, 0.0);          // touched here: first-touch faults (F1-38) are paid now
    for (std::size_t k = 0; k < n * n; ++k) {
        a[k] = static_cast<double>(k % 7);
    }
    std::printf("matrix %zu x %zu doubles = %zu MiB per matrix; one timed run each\n", n, n,
                n * n * sizeof(double) >> 20);
    std::printf("%-22s %10s\n", "loop nest", "ms");
    auto t0 = Clock::now();
    const double s1 = sumRows(a, n);
    auto t1 = Clock::now();
    std::printf("%-22s %10.1f\n", "sum by rows", ms(t0, t1));
    t0 = Clock::now();
    const double s2 = sumCols(a, n);
    t1 = Clock::now();
    std::printf("%-22s %10.1f\n", "sum by columns", ms(t0, t1));
    t0 = Clock::now();
    transposeNaive(a, b, n);
    t1 = Clock::now();
    std::printf("%-22s %10.1f\n", "transpose naive", ms(t0, t1));
    double check = b[n + 2] + b[5 * n + 4000];
    for (const std::size_t tile : {std::size_t{16}, std::size_t{64}}) {
        t0 = Clock::now();
        transposeBlocked(a, b, n, tile);
        t1 = Clock::now();
        std::printf("transpose blocked %-4zu %10.1f\n", tile, ms(t0, t1));
        check += b[n + 2] + b[5 * n + 4000];
    }
    std::printf("(sums %.0f %.0f, check %.0f)\n", s1, s2, check);
    return s1 == s2 ? 0 : 1;
}
