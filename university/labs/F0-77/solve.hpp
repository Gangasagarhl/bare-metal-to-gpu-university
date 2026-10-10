// F0-77 shared code: Gaussian elimination (with or without partial pivoting) and back substitution.
#pragma once
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <utility>
#include <vector>

using Vec = std::vector<double>;
using Mat = std::vector<Vec>; // row-major: A[i][j] is row i, column j

inline void printMat(const char* title, const Mat& A, const Vec& b)
{
    std::printf("%s\n", title);
    for (std::size_t i = 0; i < A.size(); ++i) {
        std::printf("  [");
        for (double v : A[i]) {
            std::printf(" %9.4g", v);
        }
        std::printf(" ] | %9.4g\n", b[i]);
    }
}

// Solves A x = b. A and b are taken by value: elimination overwrites them.
inline Vec solve(Mat A, Vec b, bool pivot, bool verbose)
{
    const std::size_t n = A.size();
    for (std::size_t k = 0; k + 1 < n; ++k) {
        if (pivot) { // partial pivoting: largest |A[i][k]| for i >= k goes to row k
            std::size_t p = k;
            for (std::size_t i = k + 1; i < n; ++i) {
                if (std::fabs(A[i][k]) > std::fabs(A[p][k])) {
                    p = i;
                }
            }
            if (p != k) {
                std::swap(A[p], A[k]);
                std::swap(b[p], b[k]);
                if (verbose) {
                    std::printf("step %zu: swap rows %zu and %zu (pivot %g)\n", k + 1, k + 1, p + 1,
                                A[k][k]);
                }
            }
        }
        for (std::size_t i = k + 1; i < n; ++i) {
            const double m = A[i][k] / A[k][k]; // multiplier l_ik
            for (std::size_t j = k; j < n; ++j) {
                A[i][j] -= m * A[k][j];
            }
            b[i] -= m * b[k];
            if (verbose) {
                std::printf("step %zu: row %zu -= %g x row %zu\n", k + 1, i + 1, m, k + 1);
            }
        }
    }
    if (verbose) {
        printMat("upper triangular system U x = c:", A, b);
    }
    Vec x(n);
    for (std::size_t i = n; i-- > 0;) { // back substitution, last row first
        double s = b[i];
        for (std::size_t j = i + 1; j < n; ++j) {
            s -= A[i][j] * x[j];
        }
        x[i] = s / A[i][i];
    }
    return x;
}

// Residual r = b - A x, and its largest component.
inline double residualMax(const Mat& A, const Vec& x, const Vec& b)
{
    double worst = 0.0;
    for (std::size_t i = 0; i < A.size(); ++i) {
        double r = b[i];
        for (std::size_t j = 0; j < x.size(); ++j) {
            r -= A[i][j] * x[j];
        }
        worst = std::fmax(worst, std::fabs(r));
    }
    return worst;
}
