// F10-08 Listing 3: GNSS latitude/longitude -> metres north/east of a home point,
// with a flat-Earth (equirectangular) approximation that is adequate over short
// distances. The Earth radius below is a mean value typed in for this exercise; the
// GNSS fixes are invented and contain a deliberate jump.
#include <cmath>
#include <cstdio>
#include <numbers>
#include <vector>

constexpr double kEarthRadius = 6371000.0;  // m, mean radius (check the chapter's source)
constexpr double kDeg = std::numbers::pi / 180.0;

struct Fix
{
    double t;     // s
    double lat;   // degrees
    double lon;   // degrees
};

int main()
{
    const Fix home{0.0, 45.0000640, 10.0000940};
    const double north1e7 = 1e-7 * kDeg * kEarthRadius;
    const double east1e7 = north1e7 * std::cos(home.lat * kDeg);
    std::printf("1e-7 degree is %.2f cm north-south and %.2f cm east-west at latitude %.4f\n",
                100 * north1e7, 100 * east1e7, home.lat);

    const std::vector<Fix> log{{1.0, 45.0000641, 10.0000942}, {2.0, 45.0000650, 10.0000935},
                               {3.0, 45.0000636, 10.0000948}, {4.0, 45.0000645, 10.0000939},
                               {5.0, 45.0001010, 10.0001410}, {6.0, 45.0000643, 10.0000944}};
    std::printf("   t  north m   east m   jump m\n");
    double prevN = 0.0, prevE = 0.0;
    for (const auto& f : log) {
        const double n = (f.lat - home.lat) * kDeg * kEarthRadius;
        const double e = (f.lon - home.lon) * kDeg * kEarthRadius * std::cos(home.lat * kDeg);
        const double jump = std::hypot(n - prevN, e - prevE);
        std::printf("%4.0f %8.2f %8.2f %8.2f%s\n", f.t, n, e, jump,
                    jump > 2.0 ? "   <- a stationary vehicle cannot move this far in 1 s" : "");
        prevN = n;
        prevE = e;
    }
    return 0;
}
