// F9-11 Listing 2: read a recorded scan log and draw it from above as text.
// Each valid range r at beam angle a becomes a point (r cos a, r sin a) in the sensor
// frame, then is moved into the robot frame by the mount offset from the log's header.
#include <cmath>
#include <cstdio>
#include <iostream>
#include <numbers>
#include <string>
#include <vector>

int main()
{
    std::string word;
    double mx = 0.0, my = 0.0, myaw = 0.0, t = 0.0;
    double amin = 0.0, ainc = 0.0, rmin = 0.0, rmax = 0.0;
    int count = 0;
    std::vector<double> ranges;
    while (std::cin >> word) {
        if (word == "#") {
            std::getline(std::cin, word);  // skip the comment line
        } else if (word == "mount") {
            std::cin >> word >> mx >> word >> my >> word >> myaw;
        } else if (word == "scan") {
            std::cin >> word >> t >> word >> amin >> word >> ainc >> word >> rmin >> word >> rmax >>
                word >> count;
        } else if (word == "end") {
            break;
        } else {
            ranges.push_back(std::stod(word));
        }
    }
    if (count <= 0 || static_cast<int>(ranges.size()) != count) {
        std::printf("bad log: header says %d ranges, found %zu\n", count, ranges.size());
        return 1;
    }

    const int cols = 45, rows = 33;              // 0.1 m per character
    const double x0 = -1.7, y0 = 2.0, cell = 0.1;  // top-left corner of the drawing, robot frame
    std::vector<std::string> grid(rows, std::string(cols, ' '));
    auto put = [&](double x, double y, char c) {
        const int col = static_cast<int>(std::floor((x - x0) / cell));
        const int row = static_cast<int>(std::floor((y0 - y) / cell));
        if (col >= 0 && col < cols && row >= 0 && row < rows) grid[row][col] = c;
    };
    int valid = 0, invalid = 0;
    double nearest = 1e9, nearestDeg = 0.0;
    const double rad = std::numbers::pi / 180.0;
    for (int i = 0; i < count; ++i) {
        const double r = ranges[i];
        const double a = (amin + i * ainc) * rad;
        if (r < rmin || r > rmax) {  // includes 0 = no return
            ++invalid;
            continue;
        }
        ++valid;
        const double xs = r * std::cos(a), ys = r * std::sin(a);           // sensor frame
        const double c = std::cos(myaw * rad), s = std::sin(myaw * rad);
        const double xr = mx + c * xs - s * ys, yr = my + s * xs + c * ys;  // robot frame
        put(xr, yr, '#');
        if (r < nearest) {
            nearest = r;
            nearestDeg = amin + i * ainc;
        }
    }
    put(0.0, 0.0, 'R');
    put(mx, my, 'L');
    std::printf("scan at t=%.3f s: %d beams, %d valid, %d without return\n", t, count, valid,
                invalid);
    std::printf("nearest return %.3f m at beam angle %.1f deg (sensor frame)\n", nearest,
                nearestDeg);
    std::printf("top view, robot frame, x to the right, y up, 0.1 m per character; "
                "R = robot centre, L = LiDAR\n");
    for (const std::string& line : grid) {
        std::printf("|%s|\n", line.c_str());
    }
    return 0;
}
