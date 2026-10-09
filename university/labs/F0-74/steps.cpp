// F0-74 worked example check: how much one "t += 0.01f" really adds, in different binades of t.
#include <cmath>
#include <cstdio>
#include <limits>

int main()
{
    const float dt = 0.01f;
    std::printf("%-9s %-12s %-14s %-12s %s\n", "t", "ulp(t)", "dt / ulp(t)", "real step",
                "step error %");
    const float ts[] = {1.0f, 3000.0f, 5000.0f, 10000.0f, 20000.0f, 40000.0f, 70000.0f, 140000.0f};
    for (float t : ts) {
        const float ulp = std::nextafter(t, std::numeric_limits<float>::infinity()) - t;
        const float step = (t + dt) - t; // exact: the difference of two nearby floats
        std::printf("%-9.0f %-12.6g %-14.4f %-12.9g %+.3f\n", static_cast<double>(t),
                    static_cast<double>(ulp), static_cast<double>(dt / ulp),
                    static_cast<double>(step),
                    (static_cast<double>(step) - static_cast<double>(dt)) /
                        static_cast<double>(dt) * 100.0);
    }
    return 0;
}
