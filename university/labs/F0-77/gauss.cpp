// F0-77 Listing 1: solve a 3 x 3 system by Gaussian elimination with partial pivoting, step by
// step.
#include <cstdio>

#include "solve.hpp"

int main()
{
    const Mat A = {{1, 2, 2}, {4, 4, 2}, {4, 6, 4}};
    const Vec b = {3, 4, 6};
    printMat("A | b:", A, b);
    const Vec x = solve(A, b, true, true);
    std::printf("x = (%g, %g, %g)\n", x[0], x[1], x[2]);
    std::printf("largest |residual| = %g\n", residualMax(A, x, b));
    return 0;
}
