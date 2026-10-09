// F1-68 Listing 1: the pinhole camera model.
// A point (X, Y, Z) in the camera frame (Z forward, X right, Y down, metres)
// lands on pixel u = fx * X / Z + cx, v = fy * Y / Z + cy.
// fx = fy = 600 px, cx = 320, cy = 240 (a 640 x 480 image) are pretend values;
// a real camera's numbers come from calibrating it.
#include <cstdio>
#include <initializer_list>

struct Pixel
{
    bool visible;
    double u, v;
};

Pixel project(double x, double y, double z)
{
    const double fx = 600.0;
    const double fy = 600.0;
    const double cx = 320.0;
    const double cy = 240.0;
    if (z <= 0.0) {
        return {false, 0.0, 0.0};  // behind the camera: no image
    }
    return {true, fx * x / z + cx, fy * y / z + cy};
}

int main()
{
    std::printf("a pole 1.0 m tall, 0.5 m to the right, top 0.5 m above the camera axis\n");
    std::printf("%6s %10s %10s %12s\n", "Z m", "top v px", "foot v px", "height px");
    for (double z : {1.0, 2.0, 4.0, 8.0}) {
        const Pixel top = project(0.5, -0.5, z);
        const Pixel foot = project(0.5, 0.5, z);
        std::printf("%6.1f %10.1f %10.1f %12.1f   (u = %.1f)\n", z, top.v, foot.v,
                    foot.v - top.v, top.u);
    }
    const Pixel behind = project(0.0, 0.0, -1.0);
    std::printf("point behind the camera visible? %s\n", behind.visible ? "yes" : "no");
    std::printf("\nback-projection: pixel (440, 240) is the ray X/Z = %.3f, Y/Z = %.3f\n",
                (440.0 - 320.0) / 600.0, (240.0 - 240.0) / 600.0);
    std::printf("any point on that ray, e.g. Z = 3 m -> X = %.2f m\n",
                3.0 * (440.0 - 320.0) / 600.0);
    return 0;
}
