// F0-52 Listing 2: a 3x3 rotation utility with unit tests (the shape of MA201's practical exam).
#include <array>
#include <cmath>
#include <iostream>
#include <numbers>

using Vec3 = std::array<double, 3>;
using Mat3 = std::array<std::array<double, 3>, 3>;  // m[row][col]

Mat3 rotX(double t)
{
    const double c = std::cos(t), s = std::sin(t);
    return {{{1, 0, 0}, {0, c, -s}, {0, s, c}}};
}

Mat3 rotY(double t)
{
    const double c = std::cos(t), s = std::sin(t);
    return {{{c, 0, s}, {0, 1, 0}, {-s, 0, c}}};
}

Mat3 rotZ(double t)
{
    const double c = std::cos(t), s = std::sin(t);
    return {{{c, -s, 0}, {s, c, 0}, {0, 0, 1}}};
}

Mat3 multiply(const Mat3& a, const Mat3& b)
{
    Mat3 c{};
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            for (int p = 0; p < 3; ++p) {
                c[i][j] += a[i][p] * b[p][j];
            }
        }
    }
    return c;
}

Vec3 times(const Mat3& m, const Vec3& v)
{
    Vec3 r{};
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            r[i] += m[i][j] * v[j];
        }
    }
    return r;
}

Mat3 transpose(const Mat3& m)
{
    Mat3 t{};
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            t[i][j] = m[j][i];
        }
    }
    return t;
}

double det(const Mat3& m)
{
    return m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1])
         - m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0])
         + m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
}

bool near(const Mat3& a, const Mat3& b, double tol)
{
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (std::abs(a[i][j] - b[i][j]) > tol) {
                return false;
            }
        }
    }
    return true;
}

bool near(const Vec3& a, const Vec3& b, double tol)
{
    for (int i = 0; i < 3; ++i) {
        if (std::abs(a[i] - b[i]) > tol) {
            return false;
        }
    }
    return true;
}

bool isRotation(const Mat3& m, double tol)  // R^T R = I and det R = +1
{
    const Mat3 identity{{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}};
    return near(multiply(transpose(m), m), identity, tol) && std::abs(det(m) - 1.0) <= tol;
}

int failures = 0;

void expect(bool ok, const char* what)
{
    std::cout << (ok ? "PASS  " : "FAIL  ") << what << "\n";
    if (!ok) {
        ++failures;
    }
}

int main()
{
    const double tol = 1e-12;
    const double quarter = std::numbers::pi / 2;
    const Vec3 ex{1, 0, 0}, ey{0, 1, 0}, ez{0, 0, 1};

    expect(near(times(rotZ(quarter), ex), ey, tol), "rotZ(90 deg) turns x into y");
    expect(near(times(rotX(quarter), ey), ez, tol), "rotX(90 deg) turns y into z");
    expect(near(times(rotY(quarter), ez), ex, tol), "rotY(90 deg) turns z into x");
    expect(isRotation(rotX(0.3), tol) && isRotation(rotY(-1.1), tol) && isRotation(rotZ(2.5), tol),
           "rotX, rotY, rotZ are rotations (R^T R = I, det = +1)");
    const Mat3 r = multiply(rotZ(0.4), multiply(rotY(-0.7), rotX(1.2)));
    expect(isRotation(r, tol), "a product of rotations is a rotation");
    expect(near(multiply(transpose(r), r), multiply(r, transpose(r)), tol), "R^T R equals R R^T (both are I)");
    expect(near(multiply(rotZ(0.25), rotZ(0.5)), rotZ(0.75), tol), "rotZ(a) rotZ(b) = rotZ(a + b)");
    expect(!near(multiply(rotZ(quarter), rotX(quarter)), multiply(rotX(quarter), rotZ(quarter)), 1e-6),
           "rotZ rotX differs from rotX rotZ (order matters in 3D)");
    const Mat3 mirror{{{1, 0, 0}, {0, 1, 0}, {0, 0, -1}}};
    expect(!isRotation(mirror, tol), "a mirror (det = -1) is rejected");
    const Vec3 p{3, -1, 2};
    const Vec3 q = times(r, p);
    expect(std::abs(std::hypot(q[0], q[1], q[2]) - std::hypot(p[0], p[1], p[2])) < tol,
           "rotation keeps the length of (3, -1, 2)");

    std::cout << "rotZ(90) rotX(90) applied to y: ";
    const Vec3 a = times(multiply(rotZ(quarter), rotX(quarter)), ey);
    const Vec3 b = times(multiply(rotX(quarter), rotZ(quarter)), ey);
    std::cout << "(" << std::round(a[0]) + 0.0 << ", " << std::round(a[1]) + 0.0 << ", "
              << std::round(a[2]) + 0.0 << "); rotX(90) rotZ(90) applied to y: (" << std::round(b[0]) + 0.0
              << ", " << std::round(b[1]) + 0.0 << ", " << std::round(b[2]) + 0.0 << ")\n";
    std::cout << failures << " failure(s)\n";
    return failures == 0 ? 0 : 1;
}
