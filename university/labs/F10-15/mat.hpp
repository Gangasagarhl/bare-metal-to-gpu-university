// mat.hpp: the smallest matrix type the Kalman filter chapters need (a copy of RB302's
// F9-33 mat.hpp, kept identical below this comment so the DN301 labs build on their own).
// Row-major storage in a std::vector; sizes are checked at run time. Clear rather than fast.
#pragma once
#include <cmath>
#include <initializer_list>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

struct Mat
{
    int rows = 0;
    int cols = 0;
    std::vector<double> a;

    Mat(int r, int c) : rows(r), cols(c), a(static_cast<std::size_t>(r * c), 0.0) {}
    Mat(int r, int c, std::initializer_list<double> values) : Mat(r, c)
    {
        if (values.size() != a.size()) {
            throw std::invalid_argument("Mat: wrong number of values");
        }
        std::size_t i = 0;
        for (double v : values) {
            a[i++] = v;
        }
    }
    double& operator()(int i, int j) { return a[static_cast<std::size_t>(i * cols + j)]; }
    double operator()(int i, int j) const { return a[static_cast<std::size_t>(i * cols + j)]; }

    static Mat identity(int n)
    {
        Mat m(n, n);
        for (int i = 0; i < n; ++i) {
            m(i, i) = 1.0;
        }
        return m;
    }
    Mat t() const
    {
        Mat m(cols, rows);
        for (int i = 0; i < rows; ++i) {
            for (int j = 0; j < cols; ++j) {
                m(j, i) = (*this)(i, j);
            }
        }
        return m;
    }
};

inline Mat operator*(const Mat& x, const Mat& y)
{
    if (x.cols != y.rows) {
        throw std::invalid_argument("Mat *: size mismatch");
    }
    Mat m(x.rows, y.cols);
    for (int i = 0; i < x.rows; ++i) {
        for (int j = 0; j < y.cols; ++j) {
            double s = 0.0;
            for (int k = 0; k < x.cols; ++k) {
                s += x(i, k) * y(k, j);
            }
            m(i, j) = s;
        }
    }
    return m;
}

inline Mat operator*(double s, const Mat& x)
{
    Mat m = x;
    for (double& v : m.a) {
        v *= s;
    }
    return m;
}

inline Mat operator+(const Mat& x, const Mat& y)
{
    if (x.rows != y.rows || x.cols != y.cols) {
        throw std::invalid_argument("Mat +: size mismatch");
    }
    Mat m = x;
    for (std::size_t i = 0; i < m.a.size(); ++i) {
        m.a[i] += y.a[i];
    }
    return m;
}

inline Mat operator-(const Mat& x, const Mat& y)
{
    return x + (-1.0) * y;
}

// Inverse by Gauss-Jordan elimination with partial pivoting (small matrices only).
inline Mat inverse(Mat m)
{
    if (m.rows != m.cols) {
        throw std::invalid_argument("inverse: not square");
    }
    const int n = m.rows;
    Mat inv = Mat::identity(n);
    for (int col = 0; col < n; ++col) {
        int pivot = col;
        for (int r = col + 1; r < n; ++r) {
            if (std::fabs(m(r, col)) > std::fabs(m(pivot, col))) {
                pivot = r;
            }
        }
        if (std::fabs(m(pivot, col)) < 1e-15) {
            throw std::runtime_error("inverse: matrix is singular");
        }
        for (int j = 0; j < n; ++j) {
            std::swap(m(col, j), m(pivot, j));
            std::swap(inv(col, j), inv(pivot, j));
        }
        const double d = m(col, col);
        for (int j = 0; j < n; ++j) {
            m(col, j) /= d;
            inv(col, j) /= d;
        }
        for (int r = 0; r < n; ++r) {
            if (r != col) {
                const double f = m(r, col);
                for (int j = 0; j < n; ++j) {
                    m(r, j) -= f * m(col, j);
                    inv(r, j) -= f * inv(col, j);
                }
            }
        }
    }
    return inv;
}

inline void print(const char* name, const Mat& m)
{
    const std::string pad(std::string(name).size() + 4, ' ');  // lines up the rows
    std::cout << name << " =";
    for (int i = 0; i < m.rows; ++i) {
        std::cout << (i == 0 ? std::string(" [") : "\n" + pad);
        for (int j = 0; j < m.cols; ++j) {
            std::cout << std::setw(12) << std::setprecision(6) << std::fixed << m(i, j);
        }
    }
    std::cout << " ]\n";
}
