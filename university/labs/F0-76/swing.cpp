// F0-76 forensic evidence: a game's playground swing, stepped once per frame with explicit Euler
// (the deliberate mistake). Logged: the highest angle of each swing, at 60 and at 30 frames per
// second.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <numbers>
#include <vector>

#include "pendulum.hpp"

// Peak |angle| of each forward swing (between two forward turning points), for the first `swings`
// swings.
std::vector<double> peaks(double fps, int swings)
{
    const double h = 1.0 / fps;
    State s{0.5, 0.0}; // pushed to 0.5 rad, then let go; the model has no friction
    std::vector<double> out;
    double peak = 0.0;
    double prevOmega = 0.0;
    for (int frame = 1; frame <= static_cast<int>(120.0 * fps); ++frame) {
        s = eulerStep(s, h);
        peak = std::fmax(peak, std::fabs(s.theta));
        if (std::fabs(s.theta) > std::numbers::pi) { // went over the top of the bar
            out.push_back(-1.0);
            break;
        }
        if (prevOmega > 0.0 && s.omega <= 0.0) {
            out.push_back(peak);
            peak = 0.0;
            if (static_cast<int>(out.size()) == swings) {
                break;
            }
        }
        prevOmega = s.omega;
    }
    return out;
}

int main()
{
    const std::vector<double> a = peaks(60.0, 12), b = peaks(30.0, 12);
    std::printf("swing_log: release angle 0.500 rad, peak |angle| per swing (rad); -- = swing went "
                "over the top\n");
    std::printf("%-6s %-12s %-12s\n", "swing", "60 fps", "30 fps");
    for (std::size_t i = 0; i < std::max(a.size(), b.size()); ++i) {
        auto cell = [](const std::vector<double>& v, std::size_t k) {
            if (k >= v.size()) {
                return std::printf("%-12s ", "");
            }
            if (v[k] < 0.0) {
                return std::printf("%-12s ", "-- (over)");
            }
            return std::printf("%-12.3f ", v[k]);
        };
        std::printf("%-6zu ", i + 1);
        cell(a, i);
        cell(b, i);
        std::printf("\n");
    }
    return 0;
}
