// F0-77 forensic evidence: the nightly joint-offset calibration of four robot arms.
// Each arm solves a 3 x 3 system M x = d for its three joint offsets (radians).
// The solver in the arm firmware does NOT pivot (the deliberate mistake; see the answer key).
#include <cstdio>

#include "solve.hpp"

int main()
{
    struct Arm
    {
        const char* name;
        Mat M;
        Vec d;
    };
    const Arm arms[] = {
        {"arm-A", {{2.0, 1.0, 0.5}, {1.0, 3.0, 1.0}, {0.5, 1.0, 2.5}}, {0.0245, 0.0310, 0.0180}},
        {"arm-B", {{1.8, 0.9, 0.4}, {0.9, 2.7, 1.1}, {0.4, 1.1, 2.2}}, {0.0201, 0.0122, -0.0090}},
        {"arm-C",
         {{3.0e-17, 1.0, 1.0}, {1.0, 1.0, 0.0}, {1.0, 0.0, 1.0}},
         {0.0300, 0.0200, 0.0150}},
        {"arm-D", {{2.2, 1.0, 0.3}, {1.0, 2.9, 0.8}, {0.3, 0.8, 2.4}}, {-0.0110, 0.0205, 0.0170}},
    };
    std::printf("%-6s %-12s %-12s %-12s %-14s %s\n", "arm", "offset 1", "offset 2", "offset 3",
                "max|residual|", "status");
    for (const Arm& a : arms) {
        const Vec x = solve(a.M, a.d, false, false);
        const double r = residualMax(a.M, x, a.d);
        std::printf("%-6s %-12.6f %-12.6f %-12.6f %-14.3g %s\n", a.name, x[0], x[1], x[2], r,
                    r < 1e-9 ? "ok" : "CHECK");
    }
    std::printf("M of arm-C, first row: %g %g %g\n", arms[2].M[0][0], arms[2].M[0][1],
                arms[2].M[0][2]);
    return 0;
}
