// F0-51 forensic evidence: an "undo" for a robot's map distortion. Contains ONE deliberate mistake.
#include <cmath>
#include <iomanip>
#include <iostream>

struct Mat2
{
    double a, b;
    double c, d;
};

Mat2 multiply(Mat2 m, Mat2 n)
{
    return {m.a * n.a + m.b * n.c, m.a * n.b + m.b * n.d,
            m.c * n.a + m.d * n.c, m.c * n.b + m.d * n.d};
}

Mat2 inverse(Mat2 m)
{
    const double det = m.a * m.d - m.b * m.c;
    return {m.a / det, -m.b / det,
            -m.c / det, m.d / det};
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
    std::cout << std::fixed << std::setprecision(3);
    const Mat2 testCase{2, 1, 1, 2};
    print("test case M         ", testCase);
    print("M * undo(M)         ", multiply(testCase, inverse(testCase)));
    const Mat2 wheelDistortion{4, 7, 2, 6};
    print("robot distortion D  ", wheelDistortion);
    print("D * undo(D)         ", multiply(wheelDistortion, inverse(wheelDistortion)));
    const Mat2 stretch{3, 0, 0, 0.5};
    print("stretch S           ", stretch);
    print("S * undo(S)         ", multiply(stretch, inverse(stretch)));
    return 0;
}
