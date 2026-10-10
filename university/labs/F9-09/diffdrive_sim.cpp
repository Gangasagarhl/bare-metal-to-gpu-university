// F9-09 Listing 1: the university's differential-drive simulator. It drives a square
// (1 m forward, turn 90 degrees left in place, four times) and records what the kit's
// encoders would report: cumulative counts of the left and right wheel every 0.1 s.
// Wheel, track and encoder values are PRETEND exercise values.
#include <cmath>
#include <cstdio>
#include <numbers>

int main()
{
    const double wheelDiameter = 0.070;  // m (true value, equal on both wheels here)
    const double track = 0.160;          // m, distance between the wheel contact points
    const double cpr = 1440.0;           // counts per wheel revolution (x4 decoding)
    const double dt = 0.1;               // s between log lines
    const double v = 0.25;               // m/s while driving straight
    const double w = std::numbers::pi / 4.0;  // rad/s while turning in place

    const double mPerCount = std::numbers::pi * wheelDiameter / cpr;
    double sL = 0.0;  // true distance rolled by each wheel, m
    double sR = 0.0;
    double t = 0.0;
    std::printf("# recorded encoder log: t_s left_counts right_counts\n");
    std::printf("%.1f %ld %ld\n", t, 0L, 0L);
    for (int side = 0; side < 4; ++side) {
        // forward 1 m takes 4.0 s; turning 90 degrees takes 2.0 s
        for (int k = 0; k < 40; ++k) {
            sL += v * dt;
            sR += v * dt;
            t += dt;
            std::printf("%.1f %ld %ld\n", t, std::lround(sL / mPerCount),
                        std::lround(sR / mPerCount));
        }
        for (int k = 0; k < 20; ++k) {
            sL -= w * track / 2.0 * dt;
            sR += w * track / 2.0 * dt;
            t += dt;
            std::printf("%.1f %ld %ld\n", t, std::lround(sL / mPerCount),
                        std::lround(sR / mPerCount));
        }
    }
    return 0;
}
