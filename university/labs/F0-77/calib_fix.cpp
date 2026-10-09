// F0-77 forensic answer-key run: arm-C solved again with partial pivoting.
#include <cstdio>

#include "solve.hpp"

int main()
{
    const Mat M = {{3.0e-17, 1.0, 1.0}, {1.0, 1.0, 0.0}, {1.0, 0.0, 1.0}};
    const Vec d = {0.0300, 0.0200, 0.0150};
    for (int p = 0; p < 2; ++p) {
        const Vec x = solve(M, d, p == 1, false);
        std::printf("%-17s x = (%.6f, %.6f, %.6f)  max|residual| = %.3g\n",
                    p == 1 ? "with pivoting" : "without pivoting", x[0], x[1], x[2],
                    residualMax(M, x, d));
    }
    return 0;
}
