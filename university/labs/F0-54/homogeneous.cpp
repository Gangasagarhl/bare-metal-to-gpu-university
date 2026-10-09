// F0-54 Listing 1: homogeneous transforms. A rotation plus a shift packed into one matrix,
// so that a chain of frames (world <- robot <- sensor) is a matrix product.
#include <array>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <numbers>

template <std::size_t N>
using Mat = std::array<std::array<double, N>, N>;  // N x N, row-major
template <std::size_t N>
using Vec = std::array<double, N>;

template <std::size_t N>
Mat<N> operator*(const Mat<N>& a, const Mat<N>& b)
{
    Mat<N> c{};
    for (std::size_t i = 0; i < N; ++i)
        for (std::size_t j = 0; j < N; ++j)
            for (std::size_t k = 0; k < N; ++k)
                c[i][j] += a[i][k] * b[k][j];
    return c;
}

template <std::size_t N>
Vec<N> operator*(const Mat<N>& a, const Vec<N>& v)
{
    Vec<N> r{};
    for (std::size_t i = 0; i < N; ++i)
        for (std::size_t k = 0; k < N; ++k)
            r[i] += a[i][k] * v[k];
    return r;
}

double radians(double degrees) { return degrees * std::numbers::pi / 180.0; }

// 2D pose (x, y, theta) as a 3 x 3 homogeneous matrix [R t; 0 0 1]
Mat<3> pose2(double x, double y, double thetaDeg)
{
    const double c = std::cos(radians(thetaDeg)), s = std::sin(radians(thetaDeg));
    return {{{c, -s, x}, {s, c, y}, {0, 0, 1}}};
}

// inverse of a 2D homogeneous transform: [R^T  -R^T t; 0 0 1]
Mat<3> inverse2(const Mat<3>& T)
{
    const double c = T[0][0], s = T[1][0], x = T[0][2], y = T[1][2];
    return {{{c, s, -(c * x + s * y)}, {-s, c, -(-s * x + c * y)}, {0, 0, 1}}};
}

double tidy(double v) { return std::abs(v) < 5e-13 ? 0.0 : v; }

template <std::size_t N>
void print(const char* name, const Mat<N>& m)
{
    std::cout << name << " =\n";
    for (const auto& row : m) {
        std::cout << "   ";
        for (double v : row) std::cout << std::setw(9) << tidy(v);
        std::cout << "\n";
    }
}

template <std::size_t N>
void print(const char* name, const Vec<N>& v)
{
    std::cout << name << " = (";
    for (std::size_t i = 0; i < N; ++i) std::cout << tidy(v[i]) << (i + 1 < N ? ", " : ")\n");
}

int main()
{
    std::cout << std::fixed << std::setprecision(4);

    // 1. The chain world <- robot <- sensor (2D)
    const Mat<3> T_WR = pose2(2.0, 1.0, 30.0);   // robot at (2, 1), heading 30 deg
    const Mat<3> T_RS = pose2(0.2, 0.0, 10.0);   // sensor 0.2 m ahead of the robot origin, turned 10 deg left
    const Mat<3> T_WS = T_WR * T_RS;
    print("T_WR", T_WR);
    print("T_RS", T_RS);
    print("T_WS = T_WR * T_RS", T_WS);

    const Vec<3> cupS = {1.5, 0.0, 1.0};          // the sensor sees a cup 1.5 m straight ahead (a point: w = 1)
    print("cup in sensor frame", cupS);
    print("cup in robot frame  = T_RS * cupS", T_RS * cupS);
    print("cup in world frame  = T_WS * cupS", T_WS * cupS);

    // 2. Order matters
    print("wrong order T_RS * T_WR * cupS", (T_RS * T_WR) * cupS);

    // 3. Points (w = 1) move with the shift, directions (w = 0) do not
    const Vec<3> forwardS = {1.0, 0.0, 0.0};
    print("sensor's forward direction in world (w = 0)", T_WS * forwardS);

    // 4. The inverse undoes the transform
    const Mat<3> T_SW = inverse2(T_WS);
    print("T_SW = inverse of T_WS", T_SW);
    print("T_WS * T_SW", T_WS * T_SW);
    print("cup back in sensor frame = T_SW * cupW", T_SW * (T_WS * cupS));

    // 5. 3D: 4 x 4. A drone at (1, 2, 0.5) yawed 90 deg; a camera 0.1 m ahead, 0.05 m below
    const double c = std::cos(radians(90.0)), s = std::sin(radians(90.0));
    const Mat<4> T_WB = {{{c, -s, 0, 1.0}, {s, c, 0, 2.0}, {0, 0, 1, 0.5}, {0, 0, 0, 1}}};
    const Mat<4> T_BC = {{{1, 0, 0, 0.1}, {0, 1, 0, 0.0}, {0, 0, 1, -0.05}, {0, 0, 0, 1}}};
    const Vec<4> markC = {2.0, 0.0, 0.0, 1.0};    // a marker 2 m in front of the camera
    print("T_WC = T_WB * T_BC", T_WB * T_BC);
    print("marker in world frame", (T_WB * T_BC) * markC);
    return 0;
}
