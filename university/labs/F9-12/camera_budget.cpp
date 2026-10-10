// F9-12 Listing 1: what a camera mode costs (data rate) and what it can see
// (pixels per degree, motion blur). Modes, field of view, exposure and the link's usable
// rate are PRETEND values: real ones come from the camera's datasheet and the
// documentation of the link it uses.
#include <cmath>
#include <cstdio>
#include <initializer_list>
#include <numbers>

struct Mode
{
    const char* name;
    int width, height;
    double bytesPerPixel;  // 2 for a 16-bit YUV-like format, 1 for 8-bit grey, 3 for RGB
    double fps;
};

int main()
{
    const Mode modes[] = {{"640x480 grey 30 fps", 640, 480, 1.0, 30.0},
                          {"640x480 16-bit 30 fps", 640, 480, 2.0, 30.0},
                          {"1280x720 16-bit 30 fps", 1280, 720, 2.0, 30.0},
                          {"1920x1080 RGB 30 fps", 1920, 1080, 3.0, 30.0},
                          {"1920x1080 RGB 60 fps", 1920, 1080, 3.0, 60.0}};
    const double linkMBps = 40.0;  // usable payload rate of the pretend link, MB/s (10^6 B/s)
    std::printf("%-26s %10s %10s\n", "mode", "MB/s", "fits link?");
    for (const Mode& m : modes) {
        const double mbps = m.width * m.height * m.bytesPerPixel * m.fps / 1e6;
        std::printf("%-26s %10.2f %10s\n", m.name, mbps, mbps <= linkMBps ? "yes" : "NO");
    }

    // Geometry: horizontal field of view -> focal length in pixels (pinhole model, F1-68).
    const double hfovDeg = 70.0;
    const int width = 640;
    const double f = (width / 2.0) / std::tan(hfovDeg / 2.0 * std::numbers::pi / 180.0);
    std::printf("\nwidth %d px, horizontal FOV %.0f deg -> focal length f = %.1f px\n", width,
                hfovDeg, f);
    std::printf("pixels per degree near the image centre: %.2f\n", f * std::numbers::pi / 180.0);

    // Motion blur during one exposure: turning at w rad/s smears by about f * w * t pixels.
    std::printf("\nexposure  blur when turning 1.0 rad/s"
                "  blur passing a post 1.0 m away at 0.5 m/s\n");
    for (double expMs : {1.0, 5.0, 10.0, 20.0}) {
        const double t = expMs / 1000.0;
        std::printf("%5.0f ms %20.1f px %32.1f px\n", expMs, f * 1.0 * t, f * 0.5 * t / 1.0);
    }
    return 0;
}
