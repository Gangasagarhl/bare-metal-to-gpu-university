// F9-10 Listing 1: an IMU mounted turned on the robot. The sensor's own axes (sensor frame)
// are rotated relative to the robot's axes (body frame: x forward, y left, z up).
// We find the rotation from two calibration moments (standing still, then accelerating
// straight ahead) and use it to turn sensor readings into body-frame readings.
// Readings are PRETEND numbers in SI units (m/s^2 and rad/s).
#include <array>
#include <cmath>
#include <cstdio>

using Vec = std::array<double, 3>;
using Mat = std::array<Vec, 3>;  // rows

Vec sub(const Vec& a, const Vec& b)
{
    return {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
}
double dot(const Vec& a, const Vec& b)
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}
Vec cross(const Vec& a, const Vec& b)
{
    return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
}
Vec unit(const Vec& a)
{
    const double n = std::sqrt(dot(a, a));
    return {a[0] / n, a[1] / n, a[2] / n};
}
Vec apply(const Mat& m, const Vec& v)  // m times v: rows of m dotted with v
{
    return {dot(m[0], v), dot(m[1], v), dot(m[2], v)};
}

int main()
{
    // Calibration moment 1: robot standing still on a level floor. The accelerometer
    // measures specific force (F1-65): it reads +g along the body's "up" axis.
    const Vec still{0.02, -0.01, 9.81};
    // Calibration moment 2: robot accelerating straight ahead, still level.
    const Vec ahead{0.03, -1.21, 9.81};

    const Vec up = unit(still);                      // body z, written in sensor axes
    Vec fwd = sub(ahead, still);                     // the forward acceleration alone
    fwd = sub(fwd, {up[0] * dot(fwd, up), up[1] * dot(fwd, up), up[2] * dot(fwd, up)});
    fwd = unit(fwd);                                 // body x, made exactly perpendicular to z
    const Vec left = cross(up, fwd);                 // body y = z cross x
    const Mat sensorToBody{fwd, left, up};           // rows: body axes in sensor coordinates

    std::printf("body x (forward) in sensor axes: %6.3f %6.3f %6.3f\n", fwd[0], fwd[1], fwd[2]);
    std::printf("body y (left)    in sensor axes: %6.3f %6.3f %6.3f\n", left[0], left[1], left[2]);
    std::printf("body z (up)      in sensor axes: %6.3f %6.3f %6.3f\n\n", up[0], up[1], up[2]);

    struct Sample
    {
        const char* what;
        Vec acc;
        Vec gyro;
    };
    const Sample samples[] = {
        {"standing still", {0.02, -0.01, 9.81}, {0.0, 0.0, 0.0}},
        {"braking", {0.02, 0.79, 9.81}, {0.0, 0.0, 0.0}},
        {"turning left in place", {0.02, -0.01, 9.81}, {0.0, 0.0, 0.50}},
        {"nose tipping down", {0.02, -0.01, 9.81}, {0.30, 0.0, 0.0}},
    };
    std::printf("%-22s %-24s %-24s\n", "motion", "sensor acc (x y z)", "body acc (x y z)");
    for (const Sample& s : samples) {
        const Vec b = apply(sensorToBody, s.acc);
        std::printf("%-22s %6.2f %6.2f %6.2f     %6.2f %6.2f %6.2f\n", s.what, s.acc[0], s.acc[1],
                    s.acc[2], b[0], b[1], b[2]);
    }
    std::printf("\n%-22s %-24s %-24s\n", "motion", "sensor gyro (x y z)", "body gyro (x y z)");
    for (const Sample& s : samples) {
        const Vec b = apply(sensorToBody, s.gyro);
        std::printf("%-22s %6.2f %6.2f %6.2f     %6.2f %6.2f %6.2f\n", s.what, s.gyro[0], s.gyro[1],
                    s.gyro[2], b[0], b[1], b[2]);
    }
    return 0;
}
