// F10-08 forensic evidence generator "The compass that follows the current".
// Writes compass_log.csv: a bench test in which an electronic load draws stepped
// current through the vehicle's power wiring while the vehicle stands still facing
// 90 degrees. No motor runs and no propeller is fitted. Synthetic data: the model
// (described in the answer key) adds a field proportional to current to each compass.
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <numbers>

using Vec = std::array<double, 3>;
constexpr double kDeg = std::numbers::pi / 180.0;

struct Rng
{
    std::uint64_t s = 777;
    double uniform()
    {
        s = s * 6364136223846793005ULL + 1442695040888963407ULL;
        return static_cast<double>(s >> 11) * 0x1.0p-53;
    }
    double gauss()
    {
        const double r = std::sqrt(-2.0 * std::log(1.0 - uniform()));
        return r * std::cos(2.0 * std::numbers::pi * uniform());
    }
};

double headingLevel(const Vec& m)   // body x forward, y right, z down; vehicle level
{
    double h = std::atan2(-m[1], m[0]) / kDeg;
    return h < 0 ? h + 360.0 : h;
}

int main()
{
    const Vec earthBody{0.0, -20.0, 45.0};   // uT: field of a vehicle facing east (exercise)
    const Vec kInternal{0.55, -0.35, 0.40};  // uT per ampere at the internal compass
    const Vec kExternal{0.02, -0.01, 0.01};  // uT per ampere at the compass on the mast
    std::FILE* f = std::fopen("compass_log.csv", "w");
    if (f == nullptr) {
        std::printf("cannot write compass_log.csv\n");
        return 1;
    }
    std::fprintf(f, "t_s,current_A,heading_internal_deg,heading_external_deg\n");
    std::printf("excerpt (every 25th row):\n");
    std::printf("t_s,current_A,heading_internal_deg,heading_external_deg\n");
    Rng rng;
    int rows = 0;
    for (int i = 0; i < 250; ++i) {               // 25 s at 10 Hz, five 5-second steps
        const double t = i * 0.1;
        const double amps = 5.0 * static_cast<int>(t / 5.0) + 0.3 + 0.1 * rng.gauss();
        Vec mi{}, me{};
        for (int k = 0; k < 3; ++k) {
            mi[k] = earthBody[k] + kInternal[k] * amps + 0.25 * rng.gauss();
            me[k] = earthBody[k] + kExternal[k] * amps + 0.25 * rng.gauss();
        }
        std::fprintf(f, "%.1f,%.2f,%.2f,%.2f\n", t, amps, headingLevel(mi), headingLevel(me));
        if (i % 25 == 0) {                        // excerpt for the evidence pack
            std::printf("%.1f,%.2f,%.2f,%.2f\n", t, amps, headingLevel(mi), headingLevel(me));
        }
        ++rows;
    }
    std::fclose(f);
    std::printf("wrote compass_log.csv: %d rows, columns t_s, current_A, "
                "heading_internal_deg, heading_external_deg\n", rows);
    return 0;
}
