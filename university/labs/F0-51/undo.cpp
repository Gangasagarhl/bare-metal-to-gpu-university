// F0-51 Listing 1: transpose, identity and the inverse of a 2 x 2 matrix, each checked.
#include <cmath>
#include <iomanip>
#include <iostream>
#include <optional>

struct Mat2
{
    double a, b;  // row 0: a b
    double c, d;  // row 1: c d
};

Mat2 multiply(Mat2 m, Mat2 n)
{
    return {m.a * n.a + m.b * n.c, m.a * n.b + m.b * n.d,
            m.c * n.a + m.d * n.c, m.c * n.b + m.d * n.d};
}

Mat2 transpose(Mat2 m)
{
    return {m.a, m.c,
            m.b, m.d};
}

double determinant(Mat2 m)
{
    return m.a * m.d - m.b * m.c;
}

std::optional<Mat2> inverse(Mat2 m)
{
    const double det = determinant(m);
    if (std::abs(det) < 1e-12) {
        return std::nullopt;  // singular: no inverse exists
    }
    return Mat2{m.d / det, -m.b / det,
                -m.c / det, m.a / det};
}

double tidy(double x)
{
    return std::abs(x) < 5e-10 ? 0.0 : x;  // print -0.000 and 1e-17 as 0.000
}

void print(const char* name, Mat2 m)
{
    std::cout << std::setw(14) << name << " = [[" << tidy(m.a) << ", " << tidy(m.b) << "], ["
              << tidy(m.c) << ", " << tidy(m.d) << "]]\n";
}

int main()
{
    std::cout << std::fixed << std::setprecision(3);
    const Mat2 identity{1, 0, 0, 1};
    const Mat2 a{4, 7, 2, 6};
    print("A", a);
    print("I * A", multiply(identity, a));
    print("transpose(A)", transpose(a));
    std::cout << std::setw(14) << "det(A)" << " = " << determinant(a) << "\n";

    const std::optional<Mat2> aInv = inverse(a);
    if (aInv) {
        print("inverse(A)", *aInv);
        print("A * inverse(A)", multiply(a, *aInv));
        print("inverse(A) * A", multiply(*aInv, a));
    }

    const Mat2 flat{2, 4, 1, 2};
    std::cout << std::setw(14) << "det(F)" << " = " << determinant(flat) << ", inverse(F) "
              << (inverse(flat) ? "exists" : "does not exist") << "\n";

    const Mat2 b{1, 2, 0, 1};
    print("(AB)^T", transpose(multiply(a, b)));
    print("B^T A^T", multiply(transpose(b), transpose(a)));
    print("(AB)^-1", *inverse(multiply(a, b)));
    print("B^-1 A^-1", multiply(*inverse(b), *aInv));

    const Mat2 quarterTurn{0, -1, 1, 0};
    print("inverse(Q)", *inverse(quarterTurn));
    print("transpose(Q)", transpose(quarterTurn));
    return 0;
}
