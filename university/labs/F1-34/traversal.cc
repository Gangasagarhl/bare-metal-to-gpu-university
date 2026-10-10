// "Same work, ten times slower": add up every element of an n x n matrix stored row by row
// (row-major, as C++ arrays are), once walking along rows and once walking down columns.
// Usage: traversal <n> [rows|cols|both]. Times are measurements, not specifications.
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>

double sumRows(const std::vector<double>& m, std::size_t n)
{
    double s = 0.0;
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = 0; j < n; ++j) {
            s += m[i * n + j];      // neighbours in memory, one after another
        }
    }
    return s;
}

double sumCols(const std::vector<double>& m, std::size_t n)
{
    double s = 0.0;
    for (std::size_t j = 0; j < n; ++j) {
        for (std::size_t i = 0; i < n; ++i) {
            s += m[i * n + j];      // jumps n * 8 bytes between consecutive reads
        }
    }
    return s;
}

int main(int argc, char** argv)
{
    const std::size_t n = argc > 1 ? std::stoul(argv[1]) : 4096;
    const std::string which = argc > 2 ? argv[2] : "both";
    const std::vector<double> m(n * n, 1.0);
    std::printf("matrix %zu x %zu doubles = %zu MiB\n", n, n, n * n * sizeof(double) >> 20);
    for (const std::string order : {"rows", "cols"}) {
        if (which != "both" && which != order) {
            continue;
        }
        const auto t0 = std::chrono::steady_clock::now();
        const double s = order == "rows" ? sumRows(m, n) : sumCols(m, n);
        const auto t1 = std::chrono::steady_clock::now();
        std::printf("%s: sum %.0f in %.1f ms\n", order.c_str(), s,
                    std::chrono::duration<double, std::milli>(t1 - t0).count());
    }
    return 0;
}
