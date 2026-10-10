// F9-12 Listing 2: how a stereo depth camera's error grows with distance, and how far a
// depth pixel shifts when it is drawn into a colour camera mounted beside it.
// Z = f * B / d (F1-67). One disparity step of error dd gives dZ = Z^2 / (f * B) * dd.
// Focal length, baselines and the disparity error are PRETEND values.
#include <cstdio>
#include <initializer_list>

int main()
{
    const double f = 450.0;        // px, focal length of the rectified pair
    const double baseline = 0.05;  // m, between the two infrared cameras
    const double dd = 0.25;        // px, assumed disparity error (sub-pixel matching)
    const double rgbOffset = 0.03; // m, colour camera beside the depth origin, same direction
    std::printf("  Z (m)  disparity (px)  depth error (m)  error (%% of Z)"
                "  shift into colour image (px)\n");
    for (double z : {0.3, 0.5, 1.0, 2.0, 3.0, 5.0, 8.0}) {
        const double d = f * baseline / z;
        const double dz = z * z / (f * baseline) * dd;
        const double shift = f * rgbOffset / z;  // parallax between the two viewpoints
        std::printf("%7.1f %15.2f %16.4f %14.2f %22.1f\n", z, d, dz, 100.0 * dz / z, shift);
    }
    return 0;
}
