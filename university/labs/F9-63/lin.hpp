// lin.hpp - a tiny dense-matrix toolkit for the RB402 labs (teaching code, not a library).
// Row-major storage; sizes are checked; everything is double precision.
#pragma once
#include <algorithm>
#include <cmath>
#include <complex>
#include <cstddef>
#include <cstdio>
#include <stdexcept>
#include <utility>
#include <vector>

struct Mat
{
    std::size_t r = 0;
    std::size_t c = 0;
    std::vector<double> a;
    Mat() = default;
    Mat(std::size_t rows, std::size_t cols, double v = 0.0) : r(rows), c(cols), a(rows * cols, v) {}
    Mat(std::size_t rows, std::size_t cols, std::vector<double> v)
        : r(rows), c(cols), a(std::move(v))
    {
        if (a.size() != r * c) throw std::invalid_argument("Mat: wrong number of entries");
    }
    double& operator()(std::size_t i, std::size_t j) { return a[i * c + j]; }
    double operator()(std::size_t i, std::size_t j) const { return a[i * c + j]; }
};

inline Mat eye(std::size_t n)
{
    Mat m(n, n);
    for (std::size_t i = 0; i < n; ++i) m(i, i) = 1.0;
    return m;
}

inline Mat operator+(const Mat& x, const Mat& y)
{
    if (x.r != y.r || x.c != y.c) throw std::invalid_argument("+: size mismatch");
    Mat z = x;
    for (std::size_t k = 0; k < z.a.size(); ++k) z.a[k] += y.a[k];
    return z;
}

inline Mat operator-(const Mat& x, const Mat& y)
{
    if (x.r != y.r || x.c != y.c) throw std::invalid_argument("-: size mismatch");
    Mat z = x;
    for (std::size_t k = 0; k < z.a.size(); ++k) z.a[k] -= y.a[k];
    return z;
}

inline Mat operator*(double s, const Mat& x)
{
    Mat z = x;
    for (double& v : z.a) v *= s;
    return z;
}

inline Mat operator*(const Mat& x, const Mat& y)
{
    if (x.c != y.r) throw std::invalid_argument("*: inner sizes differ");
    Mat z(x.r, y.c);
    for (std::size_t i = 0; i < x.r; ++i)
        for (std::size_t k = 0; k < x.c; ++k)
            for (std::size_t j = 0; j < y.c; ++j) z(i, j) += x(i, k) * y(k, j);
    return z;
}

inline Mat tr(const Mat& x)  // transpose
{
    Mat z(x.c, x.r);
    for (std::size_t i = 0; i < x.r; ++i)
        for (std::size_t j = 0; j < x.c; ++j) z(j, i) = x(i, j);
    return z;
}

inline double maxAbs(const Mat& x)
{
    double m = 0.0;
    for (double v : x.a) m = std::max(m, std::fabs(v));
    return m;
}

// Gauss-Jordan elimination with partial pivoting; throws if the matrix is (numerically) singular.
inline Mat inv(const Mat& x)
{
    if (x.r != x.c) throw std::invalid_argument("inv: not square");
    const std::size_t n = x.r;
    Mat a = x;
    Mat b = eye(n);
    for (std::size_t col = 0; col < n; ++col) {
        std::size_t p = col;
        for (std::size_t i = col + 1; i < n; ++i)
            if (std::fabs(a(i, col)) > std::fabs(a(p, col))) p = i;
        if (std::fabs(a(p, col)) < 1e-14 * std::max(1.0, maxAbs(x)))
            throw std::runtime_error("inv: singular matrix");
        for (std::size_t j = 0; j < n; ++j) {
            std::swap(a(col, j), a(p, j));
            std::swap(b(col, j), b(p, j));
        }
        const double d = a(col, col);
        for (std::size_t j = 0; j < n; ++j) {
            a(col, j) /= d;
            b(col, j) /= d;
        }
        for (std::size_t i = 0; i < n; ++i) {
            if (i == col) continue;
            const double f = a(i, col);
            for (std::size_t j = 0; j < n; ++j) {
                a(i, j) -= f * a(col, j);
                b(i, j) -= f * b(col, j);
            }
        }
    }
    return b;
}

// Numerical rank: row reduction, pivots smaller than tol * (largest entry) count as zero.
inline std::size_t rank(const Mat& x, double tol = 1e-9)
{
    Mat a = x;
    const double scale = std::max(maxAbs(x), 1e-300);
    std::size_t rk = 0;
    for (std::size_t col = 0; col < a.c && rk < a.r; ++col) {
        std::size_t p = rk;
        for (std::size_t i = rk + 1; i < a.r; ++i)
            if (std::fabs(a(i, col)) > std::fabs(a(p, col))) p = i;
        if (std::fabs(a(p, col)) <= tol * scale) continue;
        for (std::size_t j = 0; j < a.c; ++j) std::swap(a(rk, j), a(p, j));
        for (std::size_t i = rk + 1; i < a.r; ++i) {
            const double f = a(i, col) / a(rk, col);
            for (std::size_t j = col; j < a.c; ++j) a(i, j) -= f * a(rk, j);
        }
        ++rk;
    }
    return rk;
}

// Matrix exponential e^X by scaling and squaring with a 20-term Taylor series.
inline Mat expm(const Mat& x)
{
    int s = 0;
    double nrm = maxAbs(x) * static_cast<double>(x.r);
    while (nrm > 0.5) {
        nrm /= 2.0;
        ++s;
    }
    const Mat y = std::ldexp(1.0, -s) * x;
    Mat term = eye(x.r);
    Mat sum = eye(x.r);
    for (int k = 1; k <= 20; ++k) {
        term = (1.0 / k) * (term * y);
        sum = sum + term;
    }
    for (int i = 0; i < s; ++i) sum = sum * sum;
    return sum;
}

// Exact zero-order-hold discretisation: [Ad Bd; 0 I] = expm([A B; 0 0] * T).
inline std::pair<Mat, Mat> c2d(const Mat& A, const Mat& B, double T)
{
    const std::size_t n = A.r;
    const std::size_t m = B.c;
    Mat big(n + m, n + m);
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = 0; j < n; ++j) big(i, j) = A(i, j) * T;
        for (std::size_t j = 0; j < m; ++j) big(i, n + j) = B(i, j) * T;
    }
    const Mat e = expm(big);
    Mat Ad(n, n);
    Mat Bd(n, m);
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = 0; j < n; ++j) Ad(i, j) = e(i, j);
        for (std::size_t j = 0; j < m; ++j) Bd(i, j) = e(i, n + j);
    }
    return {Ad, Bd};
}

// Characteristic polynomial det(sI - A) = s^n + p[1] s^(n-1) + ... + p[n]  (Faddeev-LeVerrier).
inline std::vector<double> charpoly(const Mat& A)
{
    const std::size_t n = A.r;
    std::vector<double> p(n + 1, 0.0);
    p[0] = 1.0;
    Mat M = eye(n);
    for (std::size_t k = 1; k <= n; ++k) {
        if (k > 1) M = A * M + p[k - 1] * eye(n);
        const Mat AM = A * M;
        double trace = 0.0;
        for (std::size_t i = 0; i < n; ++i) trace += AM(i, i);
        p[k] = -trace / static_cast<double>(k);
    }
    return p;
}

// All roots of a monic polynomial p[0] s^n + ... + p[n] (Durand-Kerner iteration).
inline std::vector<std::complex<double>> roots(const std::vector<double>& p)
{
    using cd = std::complex<double>;
    const std::size_t n = p.size() - 1;
    double bound = 1.0;
    for (std::size_t k = 1; k <= n; ++k) bound = std::max(bound, 1.0 + std::fabs(p[k] / p[0]));
    std::vector<cd> z(n);
    for (std::size_t i = 0; i < n; ++i)
        z[i] = bound * std::pow(cd(0.4, 0.9), static_cast<double>(i));
    auto val = [&](cd s) {
        cd v = 1.0;
        for (std::size_t k = 1; k <= n; ++k) v = v * s + p[k] / p[0];
        return v;
    };
    for (int it = 0; it < 3000; ++it) {
        for (std::size_t i = 0; i < n; ++i) {
            cd den = 1.0;
            for (std::size_t j = 0; j < n; ++j)
                if (j != i) den *= (z[i] - z[j]);
            z[i] -= val(z[i]) / den;
        }
    }
    std::sort(z.begin(), z.end(), [](cd u, cd v) {
        return u.real() != v.real() ? u.real() > v.real() : u.imag() > v.imag();
    });
    return z;
}

inline std::vector<std::complex<double>> eig(const Mat& A) { return roots(charpoly(A)); }

// Eigenvalues of a symmetric matrix by cyclic Jacobi rotations (sorted, largest first).
inline std::vector<double> symEig(Mat a)
{
    const std::size_t n = a.r;
    for (int sweep = 0; sweep < 100; ++sweep) {
        double off = 0.0;
        for (std::size_t i = 0; i < n; ++i)
            for (std::size_t j = i + 1; j < n; ++j) off += a(i, j) * a(i, j);
        if (off < 1e-30 * std::max(1.0, maxAbs(a) * maxAbs(a))) break;
        for (std::size_t p = 0; p < n; ++p) {
            for (std::size_t q = p + 1; q < n; ++q) {
                if (a(p, q) == 0.0) continue;
                const double th = 0.5 * std::atan2(2.0 * a(p, q), a(q, q) - a(p, p));
                const double cs = std::cos(th);
                const double sn = std::sin(th);
                for (std::size_t k = 0; k < n; ++k) {  // rotate columns p and q
                    const double akp = a(k, p);
                    const double akq = a(k, q);
                    a(k, p) = cs * akp - sn * akq;
                    a(k, q) = sn * akp + cs * akq;
                }
                for (std::size_t k = 0; k < n; ++k) {  // rotate rows p and q
                    const double apk = a(p, k);
                    const double aqk = a(q, k);
                    a(p, k) = cs * apk - sn * aqk;
                    a(q, k) = sn * apk + cs * aqk;
                }
            }
        }
    }
    std::vector<double> ev(n);
    for (std::size_t i = 0; i < n; ++i) ev[i] = a(i, i);
    std::sort(ev.begin(), ev.end(), [](double u, double v) { return u > v; });
    return ev;
}

inline void printMat(const char* name, const Mat& m)
{
    std::printf("%s =\n", name);
    for (std::size_t i = 0; i < m.r; ++i) {
        std::printf("  [");
        for (std::size_t j = 0; j < m.c; ++j) std::printf(" %10.4f", m(i, j));
        std::printf(" ]\n");
    }
}

inline void printEig(const char* name, const std::vector<std::complex<double>>& z)
{
    std::printf("%s:", name);
    for (const auto& v : z) {
        if (std::fabs(v.imag()) < 1e-9) std::printf("  %.4f", v.real());
        else std::printf("  %.4f%+.4fj", v.real(), v.imag());
    }
    std::printf("\n");
}
