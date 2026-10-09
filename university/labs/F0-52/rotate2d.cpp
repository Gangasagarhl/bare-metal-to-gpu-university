// F0-52 Listing 1: 2D rotation matrices: build, apply, check, combine and undo.
#include <cmath>
#include <iomanip>
#include <iostream>
#include <numbers>

struct Vec2
{
    double x;
    double y;
};

struct Mat2
{
    double a, b;  // row 0
    double c, d;  // row 1
};

double radians(double degrees)
{
    return degrees * std::numbers::pi / 180.0;
}

Mat2 rotation(double theta)  // theta in radians, anticlockwise positive
{
    const double c = std::cos(theta);
    const double s = std::sin(theta);
    return {c, -s,
            s, c};
}

Vec2 apply(Mat2 m, Vec2 v)
{
    return {m.a * v.x + m.b * v.y, m.c * v.x + m.d * v.y};
}

Mat2 multiply(Mat2 m, Mat2 n)
{
    return {m.a * n.a + m.b * n.c, m.a * n.b + m.b * n.d,
            m.c * n.a + m.d * n.c, m.c * n.b + m.d * n.d};
}

Mat2 transpose(Mat2 m)
{
    return {m.a, m.c, m.b, m.d};
}

double tidy(double x)
{
    return std::abs(x) < 5e-10 ? 0.0 : x;
}

void print(const char* name, Mat2 m)
{
    std::cout << name << " = [[" << tidy(m.a) << ", " << tidy(m.b) << "], [" << tidy(m.c) << ", "
              << tidy(m.d) << "]]\n";
}

int main()
{
    std::cout << std::fixed << std::setprecision(4);
    const Mat2 r30 = rotation(radians(30.0));
    print("R(30 deg)", r30);

    const Vec2 corners[] = {{2.0, 0.0}, {2.0, 1.0}, {0.0, 1.0}};
    for (const Vec2& p : corners) {
        const Vec2 q = apply(r30, p);
        std::cout << "(" << p.x << ", " << p.y << ") -> (" << tidy(q.x) << ", " << tidy(q.y)
                  << ")   length " << std::hypot(p.x, p.y) << " -> " << std::hypot(q.x, q.y) << "\n";
    }

    print("R^T R", multiply(transpose(r30), r30));
    std::cout << "det R = " << r30.a * r30.d - r30.b * r30.c << "\n";
    print("R(30) R(60)", multiply(r30, rotation(radians(60.0))));
    print("R(90)", rotation(radians(90.0)));
    print("R(-30)", rotation(radians(-30.0)));
    print("R(30)^T", transpose(r30));

    const Vec2 q = apply(r30, {2.0, 1.0});
    std::cout << "heading of (2, 1) before: " << std::atan2(1.0, 2.0) * 180.0 / std::numbers::pi
              << " deg, after: " << std::atan2(q.y, q.x) * 180.0 / std::numbers::pi << " deg\n";
    return 0;
}
