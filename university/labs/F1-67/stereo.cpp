// F1-67 Listing 3: depth from stereo disparity, Z = f * B / d.
// f = focal length in pixels, B = baseline between the two cameras in metres,
// d = disparity in pixels. f = 700 px and B = 0.12 m are pretend exercise values.
#include <cstdio>
#include <initializer_list>

int main()
{
    const double f = 700.0;
    const double b = 0.12;
    std::printf("depth for some disparities (f = %.0f px, B = %.2f m)\n", f, b);
    for (double d : {84.0, 42.0, 21.0, 10.5, 4.2, 1.0}) {
        std::printf("  d = %5.1f px -> Z = %6.2f m\n", d, f * b / d);
    }
    std::printf("\nwhat a half-pixel disparity error does at each depth\n");
    for (double z : {0.5, 1.0, 2.0, 4.0, 8.0}) {
        const double d = f * b / z;
        const double zNear = f * b / (d + 0.5);
        const double zFar = f * b / (d - 0.5);
        std::printf("  Z = %4.1f m: d = %6.2f px, Z range %6.3f .. %6.3f m (spread %.3f m)\n", z,
                    d, zNear, zFar, zFar - zNear);
    }
    return 0;
}
