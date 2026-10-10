// logodds.cpp - F9-52 worked example: one cell, a sequence of observations.
// A door cell is seen closed (hit) 12 times, then the door opens and the cell is
// seen free (a beam passes through) again and again. Compare clamped and unclamped.
#include <algorithm>
#include <cmath>
#include <cstdio>

double prob(double l)
{
    return 1.0 - 1.0 / (1.0 + std::exp(l));
}

int main()
{
    const double lOcc = 0.85;    // = log(0.7 / 0.3), rounded
    const double lFree = -0.40;  // = log(0.4 / 0.6), rounded
    std::printf("lOcc = log(0.7/0.3) = %.4f, lFree = log(0.4/0.6) = %.4f\n",
                std::log(0.7 / 0.3), std::log(0.4 / 0.6));
    double clamped = 0.0;
    double raw = 0.0;
    std::printf(" obs  event   l(clamped) p(clamped)   l(raw)  p(raw)\n");
    for (int t = 1; t <= 40; ++t) {
        const bool hit = t <= 12;
        const double d = hit ? lOcc : lFree;
        clamped = std::clamp(clamped + d, -4.0, 4.0);
        raw += d;
        if (t <= 3 || t == 12 || t == 13 || t % 4 == 0) {
            std::printf("%4d  %-6s  %+9.2f  %9.3f  %+8.2f  %6.3f%s\n", t, hit ? "hit" : "free",
                        clamped, prob(clamped), raw, prob(raw),
                        (!hit && prob(clamped) < 0.35 && prob(clamped - lFree) >= 0.35) ? "  <- clamped map says free" :
                        (!hit && prob(raw) < 0.35 && prob(raw - lFree) >= 0.35) ? "  <- raw map says free" : "");
        }
    }
    return 0;
}
