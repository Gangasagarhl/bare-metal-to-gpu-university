// F1-68 forensic evidence generator: "The leaning pole".
// The university's camera simulator with a ROLLING shutter: row r of the image is
// read at time r * rowTime. A thin vertical pole stands in front of the robot.
// Image A: robot still. Image B: robot turning, so the pole moves sideways in the
// image at 0.8 columns per millisecond. Row time 1.0 ms (pretend). Synthetic data.
#include <cmath>
#include <cstdio>
#include <string>

void capture(const char* title, double colsPerMs, double rowTimeMs)
{
    const int rows = 16;
    const int cols = 40;
    std::printf("%s\n", title);
    for (int r = 0; r < rows; ++r) {
        const double t = r * rowTimeMs;            // when this row is read
        const double pole = 10.0 + colsPerMs * t;  // pole's column at that time
        std::string line(cols, '.');
        for (int c = 0; c < cols; ++c) {
            if (std::fabs(c - pole) < 1.0) {
                line[c] = '#';
            }
        }
        std::printf("  row %2d (t=%4.1f ms) %s\n", r, t, line.c_str());
    }
    std::printf("\n");
}

int main()
{
    std::printf("camera: 16 rows x 40 columns, row read time 1.0 ms\n\n");
    capture("Image A: robot standing still", 0.0, 1.0);
    capture("Image B: robot turning left", 0.8, 1.0);
    return 0;
}
