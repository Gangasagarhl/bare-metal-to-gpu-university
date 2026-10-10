// F0-50 Listing 2: the same product computed tile by tile gives the same matrix (GEMM preview).
#include <cstddef>
#include <iostream>
#include <vector>

constexpr std::size_t n = 6;     // matrix size
constexpr std::size_t tile = 2;  // tile size; n is a multiple of tile here

using Mat = std::vector<double>;  // n x n, row-major

Mat naive(const Mat& a, const Mat& b)
{
    Mat c(n * n, 0.0);
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = 0; j < n; ++j) {
            for (std::size_t p = 0; p < n; ++p) {
                c[i * n + j] += a[i * n + p] * b[p * n + j];
            }
        }
    }
    return c;
}

Mat tiled(const Mat& a, const Mat& b)
{
    Mat c(n * n, 0.0);
    for (std::size_t i0 = 0; i0 < n; i0 += tile) {          // tile row of C
        for (std::size_t j0 = 0; j0 < n; j0 += tile) {      // tile column of C
            for (std::size_t p0 = 0; p0 < n; p0 += tile) {  // C tile += A tile * B tile
                for (std::size_t i = i0; i < i0 + tile; ++i) {
                    for (std::size_t j = j0; j < j0 + tile; ++j) {
                        for (std::size_t p = p0; p < p0 + tile; ++p) {
                            c[i * n + j] += a[i * n + p] * b[p * n + j];
                        }
                    }
                }
            }
        }
    }
    return c;
}

int main()
{
    Mat a(n * n);
    Mat b(n * n);
    for (std::size_t k = 0; k < n * n; ++k) {
        a[k] = static_cast<double>(k % 7) - 3.0;  // small whole numbers: exact in double
        b[k] = static_cast<double>(k % 5) - 2.0;
    }
    const Mat c1 = naive(a, b);
    const Mat c2 = tiled(a, b);
    std::cout << "first row of C:";
    for (std::size_t j = 0; j < n; ++j) {
        std::cout << " " << c1[j];
    }
    std::cout << "\n" << (c1 == c2 ? "tiled result equals naive result" : "DIFFERENT") << "\n";
    std::cout << "multiply-adds: " << n * n * n << " (n^3 for n = " << n << ")\n";
    return c1 == c2 ? 0 : 1;
}
