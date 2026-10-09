// F0-77 Listing 2: why pivoting matters. [[e, 1], [1, 1]] x = [1, 2] has x close to (1, 1) for
// small e.
#include <cmath>
#include <cstdio>

#include "solve.hpp"

int main()
{
    std::printf("%-8s %-24s %-24s %-12s %-12s\n", "e", "x without pivoting", "x with pivoting",
                "resid no-piv", "resid piv");
    for (int k = 2; k <= 20; k += 2) {
        const double e = std::pow(10.0, -k);
        const Mat A = {{e, 1}, {1, 1}};
        const Vec b = {1, 2};
        const Vec x0 = solve(A, b, false, false);
        const Vec x1 = solve(A, b, true, false);
        std::printf("%-8.0e (%-10.8f, %-10.8f) (%-10.8f, %-10.8f) %-12.3g %-12.3g\n", e, x0[0],
                    x0[1], x1[0], x1[1], residualMax(A, x0, b), residualMax(A, x1, b));
    }
    return 0;
}
