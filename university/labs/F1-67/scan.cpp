// F1-67 Listing 2: the university's 2D scanning range-sensor simulator.
// A pretend sensor at (2.0, 1.5) m in a 6 m x 4 m room with one box sends a beam
// every degree and reports the distance to the first wall or box it hits
// (ray casting), with a small deterministic noise. It then turns each
// (angle, range) pair into an (x, y) point and draws the points on a text map.
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <numbers>
#include <string>
#include <vector>

struct Segment
{
    double x1, y1, x2, y2;
};

// Distance along the ray from (px,py) in direction (dx,dy) to segment s, or -1.
double hit(double px, double py, double dx, double dy, const Segment& s)
{
    const double ex = s.x2 - s.x1;
    const double ey = s.y2 - s.y1;
    const double den = dx * ey - dy * ex;
    if (std::fabs(den) < 1e-12) {
        return -1.0;
    }
    const double t = ((s.x1 - px) * ey - (s.y1 - py) * ex) / den;
    const double u = ((s.x1 - px) * dy - (s.y1 - py) * dx) / den;
    return (t > 0.0 && u >= 0.0 && u <= 1.0) ? t : -1.0;
}

int main()
{
    const std::vector<Segment> world = {
        {0, 0, 6, 0}, {6, 0, 6, 4}, {6, 4, 0, 4}, {0, 4, 0, 0},              // room walls
        {4.0, 2.0, 4.6, 2.0}, {4.6, 2.0, 4.6, 2.6}, {4.6, 2.6, 4.0, 2.6}, {4.0, 2.6, 4.0, 2.0}};
    const double sx = 2.0;
    const double sy = 1.5;
    const double maxRange = 8.0;  // pretend
    std::uint32_t seed = 42u;
    std::vector<double> ranges(360);
    for (int a = 0; a < 360; ++a) {
        const double th = a * std::numbers::pi / 180.0;
        double best = maxRange + 1.0;
        for (const Segment& s : world) {
            const double d = hit(sx, sy, std::cos(th), std::sin(th), s);
            if (d > 0.0 && d < best) {
                best = d;
            }
        }
        seed = seed * 1664525u + 1013904223u;
        const double noise = ((seed >> 8) % 2001) / 1000.0 - 1.0;  // -1 .. +1
        ranges[a] = best <= maxRange ? best + 0.01 * noise : 0.0;
    }
    std::printf("every 30th beam (angle deg, range m, x m, y m):\n");
    for (int a = 0; a < 360; a += 30) {
        const double th = a * std::numbers::pi / 180.0;
        std::printf("  %3d  %6.3f  %6.3f  %6.3f\n", a, ranges[a], sx + ranges[a] * std::cos(th),
                    sy + ranges[a] * std::sin(th));
    }
    // Text map: 0.2 m cells, y up. '#' = at least one point, 'S' = sensor.
    const int w = 31;
    const int h = 21;
    std::vector<std::string> map(h, std::string(w, '.'));
    for (int a = 0; a < 360; ++a) {
        const double th = a * std::numbers::pi / 180.0;
        const int cx = static_cast<int>(std::lround((sx + ranges[a] * std::cos(th)) / 0.2));
        const int cy = static_cast<int>(std::lround((sy + ranges[a] * std::sin(th)) / 0.2));
        if (cx >= 0 && cx < w && cy >= 0 && cy < h) {
            map[h - 1 - cy][cx] = '#';
        }
    }
    const int sRow = h - 1 - static_cast<int>(std::lround(sy / 0.2));
    map[sRow][static_cast<int>(std::lround(sx / 0.2))] = 'S';
    std::printf("\ntext map of the 360 points (0.2 m per character, y up):\n");
    for (const std::string& row : map) {
        std::printf("  %s\n", row.c_str());
    }
    return 0;
}
