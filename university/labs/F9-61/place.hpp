// place.hpp - pole placement for one input by Ackermann's formula (F9-61).
#pragma once
#include <complex>
#include <vector>
#include "lin.hpp"

// Monic polynomial with the given roots (complex roots must come in conjugate pairs).
inline std::vector<double> polyFromRoots(const std::vector<std::complex<double>>& r)
{
    std::vector<std::complex<double>> c{1.0};
    for (const auto& z : r) {
        std::vector<std::complex<double>> d(c.size() + 1, 0.0);
        for (std::size_t k = 0; k < c.size(); ++k) {
            d[k] += c[k];
            d[k + 1] -= z * c[k];
        }
        c = d;
    }
    std::vector<double> out;
    for (const auto& v : c) out.push_back(v.real());
    return out;
}

// K such that eig(A - B K) are the desired poles:  K = [0 ... 0 1] Wc^-1 phi(A).
inline Mat acker(const Mat& A, const Mat& B, const std::vector<std::complex<double>>& poles)
{
    const std::size_t n = A.r;
    Mat Wc(n, n);
    Mat blk = B;
    for (std::size_t k = 0; k < n; ++k) {
        for (std::size_t i = 0; i < n; ++i) Wc(i, k) = blk(i, 0);
        blk = A * blk;
    }
    const auto a = polyFromRoots(poles);  // s^n + a[1] s^(n-1) + ... + a[n]
    Mat phi = eye(n);  // Horner: phi = (((A + a1 I) A + a2 I) A + ...)
    for (std::size_t k = 1; k <= n; ++k) phi = phi * A + a[k] * eye(n);
    Mat last(1, n);
    last(0, n - 1) = 1.0;
    return last * inv(Wc) * phi;
}
