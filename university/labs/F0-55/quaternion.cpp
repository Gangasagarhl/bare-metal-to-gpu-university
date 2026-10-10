// F0-55 Listing 1: unit quaternions for attitude (Hamilton convention, stored w, x, y, z).
// Builds rotations from axis and angle, rotates vectors with q v q*, composes rotations,
// converts to a rotation matrix, and shows the gimbal-lock problem of yaw-pitch-roll angles.
#include <array>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <numbers>

struct Vec3 { double x, y, z; };
struct Quat { double w, x, y, z; };
using Mat3 = std::array<std::array<double, 3>, 3>;

double radians(double d) { return d * std::numbers::pi / 180.0; }
double degrees(double r) { return r * 180.0 / std::numbers::pi; }
double tidy(double v) { return std::abs(v) < 5e-13 ? 0.0 : v; }

// rotation by angle (degrees) about a unit axis
Quat fromAxisAngle(Vec3 axis, double angleDeg)
{
    const double h = radians(angleDeg) / 2.0, s = std::sin(h);
    return {std::cos(h), axis.x * s, axis.y * s, axis.z * s};
}

// Hamilton product a * b: "do b first, then a" when used as a * v * conj
Quat operator*(const Quat& a, const Quat& b)
{
    return {a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z,
            a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
            a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
            a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w};
}

Quat conj(const Quat& q) { return {q.w, -q.x, -q.y, -q.z}; }
double norm(const Quat& q) { return std::sqrt(q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z); }

// v' = q v q*, with v written as the pure quaternion (0, v)
Vec3 rotate(const Quat& q, Vec3 v)
{
    const Quat r = q * Quat{0, v.x, v.y, v.z} * conj(q);
    return {r.x, r.y, r.z};
}

// the rotation matrix of a UNIT quaternion
Mat3 toMatrix(const Quat& q)
{
    const double w = q.w, x = q.x, y = q.y, z = q.z;
    return {{{1 - 2 * (y * y + z * z), 2 * (x * y - w * z), 2 * (x * z + w * y)},
             {2 * (x * y + w * z), 1 - 2 * (x * x + z * z), 2 * (y * z - w * x)},
             {2 * (x * z - w * y), 2 * (y * z + w * x), 1 - 2 * (x * x + y * y)}}};
}

Mat3 operator*(const Mat3& a, const Mat3& b)
{
    Mat3 c{};
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            for (int k = 0; k < 3; ++k) c[i][j] += a[i][k] * b[k][j];
    return c;
}

Mat3 rotX(double d) { const double c = std::cos(radians(d)), s = std::sin(radians(d)); return {{{1, 0, 0}, {0, c, -s}, {0, s, c}}}; }
Mat3 rotY(double d) { const double c = std::cos(radians(d)), s = std::sin(radians(d)); return {{{c, 0, s}, {0, 1, 0}, {-s, 0, c}}}; }
Mat3 rotZ(double d) { const double c = std::cos(radians(d)), s = std::sin(radians(d)); return {{{c, -s, 0}, {s, c, 0}, {0, 0, 1}}}; }

double maxDiff(const Mat3& a, const Mat3& b)
{
    double m = 0;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j) m = std::max(m, std::abs(a[i][j] - b[i][j]));
    return m;
}

void print(const char* name, const Quat& q)
{
    std::cout << name << " = (" << tidy(q.w) << ", " << tidy(q.x) << ", " << tidy(q.y) << ", " << tidy(q.z) << ")\n";
}
void print(const char* name, Vec3 v)
{
    std::cout << name << " = (" << tidy(v.x) << ", " << tidy(v.y) << ", " << tidy(v.z) << ")\n";
}

int main()
{
    std::cout << std::fixed << std::setprecision(4);
    const Vec3 ex{1, 0, 0}, ey{0, 1, 0}, ez{0, 0, 1};

    // 1. a yaw of 90 degrees about z
    const Quat yaw90 = fromAxisAngle(ez, 90);
    print("yaw90 = fromAxisAngle(z, 90)", yaw90);
    std::cout << "|yaw90| = " << norm(yaw90) << "\n";
    print("rotate(yaw90, e_x)", rotate(yaw90, ex));

    // 2. q and -q are the same rotation
    const Quat minus{-yaw90.w, -yaw90.x, -yaw90.y, -yaw90.z};
    print("rotate(-yaw90, e_x)", rotate(minus, ex));

    // 3. composing: pitch 90 about y, then yaw 90 about z  ->  q = yaw * pitch
    const Quat pitch90 = fromAxisAngle(ey, 90);
    const Quat both = yaw90 * pitch90;
    print("both = yaw90 * pitch90", both);
    print("rotate(both, e_x)", rotate(both, ex));
    print("rotate(yaw90, rotate(pitch90, e_x))", rotate(yaw90, rotate(pitch90, ex)));
    print("other order: rotate(pitch90 * yaw90, e_x)", rotate(pitch90 * yaw90, ex));

    // 4. the quaternion's matrix equals the matrix product of F0-52
    std::cout << "max |toMatrix(both) - rotZ(90)*rotY(90)| = " << std::scientific
              << maxDiff(toMatrix(both), rotZ(90) * rotY(90)) << std::fixed << "\n";
    const Quat odd = fromAxisAngle({0.48, 0.6, 0.64}, 37);  // a unit axis (0.48^2 + 0.6^2 + 0.64^2 = 1)
    const Vec3 v{0.3, -1.2, 2.0};
    const Mat3 R = toMatrix(odd);
    const Vec3 byMatrix{R[0][0] * v.x + R[0][1] * v.y + R[0][2] * v.z,
                        R[1][0] * v.x + R[1][1] * v.y + R[1][2] * v.z,
                        R[2][0] * v.x + R[2][1] * v.y + R[2][2] * v.z};
    print("37 deg about (0.48, 0.6, 0.64): q v q*", rotate(odd, v));
    print("37 deg about (0.48, 0.6, 0.64): R v   ", byMatrix);

    // 5. gimbal lock: yaw-pitch-roll R = Rz(yaw) Ry(pitch) Rx(roll) at pitch 90
    std::cout << "yaw-pitch-roll at pitch 90:\n";
    const double sets[][3] = {{30, 90, 0}, {0, 90, -30}, {50, 90, 20}, {30, 90, 10}};
    const Mat3 first = rotZ(sets[0][0]) * rotY(sets[0][1]) * rotX(sets[0][2]);
    for (const auto& a : sets) {
        const Mat3 m = rotZ(a[0]) * rotY(a[1]) * rotX(a[2]);
        std::cout << "  yaw " << std::setw(6) << a[0] << "  pitch " << a[1] << "  roll " << std::setw(7) << a[2]
                  << "  -> differs from the first by " << std::scientific << maxDiff(m, first) << std::fixed << "\n";
    }
    return 0;
}
