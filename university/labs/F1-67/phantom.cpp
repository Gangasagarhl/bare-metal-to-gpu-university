// F1-67 forensic evidence generator: "The phantom wall".
// The pretend scanner driver reports 0.0 for a beam that got no echo (nothing
// within its pretend 8 m range). A robot in a long corridor runs a safety check:
// "stop if any point is closer than 0.5 m within 30 degrees of straight ahead".
// The corridor and scan are synthetic, made here; the learner sees the log only.
#include <cmath>
#include <cstdio>
#include <numbers>
#include <vector>

int main()
{
    // Robot in the middle of a corridor 2 m wide, facing +x along it (angle 0).
    // Side walls at y = +1 and y = -1; the corridor is open far ahead.
    const double maxRange = 8.0;
    std::vector<double> ranges(360, 0.0);
    for (int a = 0; a < 360; ++a) {
        const double th = a * std::numbers::pi / 180.0;
        const double s = std::sin(th);
        double r = maxRange + 1.0;
        if (std::fabs(s) > 1e-9) {
            r = 1.0 / std::fabs(s);
        }
        ranges[a] = r <= maxRange ? r : 0.0;  // no echo -> driver reports 0.0
    }
    std::printf("scan excerpt, beams -40..+40 deg (angle, reported range m):\n");
    for (int a = -40; a <= 40; a += 5) {
        std::printf("  %+4d  %6.3f\n", a, ranges[(a + 360) % 360]);
    }
    std::printf("\nsafety check: points closer than 0.5 m within +-30 deg ahead\n");
    int close = 0;
    for (int a = -30; a <= 30; ++a) {
        const double r = ranges[(a + 360) % 360];
        const double th = a * std::numbers::pi / 180.0;
        if (r < 0.5) {
            if (close < 4) {
                std::printf("  beam %+3d deg: point at (%.2f, %.2f) m, %.2f m away\n", a,
                            r * std::cos(th), r * std::sin(th), r);
            }
            ++close;
        }
    }
    std::printf("  %d beams flagged -> decision: %s\n", close, close > 0 ? "STOP" : "GO");
    return 0;
}
