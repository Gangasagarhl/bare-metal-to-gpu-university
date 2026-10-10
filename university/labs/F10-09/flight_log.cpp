// F10-09 forensic evidence generator "Vibration". Writes flightA.csv (hover before the
// rebuild) and flightB.csv (hover after the rebuild): 2 s of accelerometer samples at
// 1000 Hz in g, body axes x forward, y left, z up. SYNTHETIC: the frame vibration, the
// mount model and the +-16 g sensor range are the university's exercise values; what
// differs between the two files is written only in the answer key.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <numbers>

struct Rng
{
    std::uint64_t s = 4242;
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

double transmissibility(double f, double fn, double zeta)
{
    if (fn <= 0) {
        return 1.0;                               // rigid mount: the IMU moves with the frame
    }
    const double r = f / fn, d = 2 * zeta * r;
    return std::sqrt(1 + d * d) / std::sqrt((1 - r * r) * (1 - r * r) + d * d);
}

bool write(const char* name, double fn, std::uint64_t seed)
{
    std::FILE* f = std::fopen(name, "w");
    if (f == nullptr) {
        return false;
    }
    Rng rng{seed};
    const double f1 = 95.0, f2 = 190.0;                  // Hz: once per turn, blade pass
    const std::array<double, 3> a1{1.0, 1.0, 1.5};       // g at the frame, per axis
    const std::array<double, 3> a2{6.0, 6.0, 18.0};
    const std::array<double, 3> gravity{0.0, 0.0, 1.0};
    const double t1 = transmissibility(f1, fn, 0.15), t2 = transmissibility(f2, fn, 0.15);
    std::fprintf(f, "t_s,ax_g,ay_g,az_g\n");
    for (int i = 0; i < 2000; ++i) {
        const double t = i / 1000.0;
        std::fprintf(f, "%.3f", t);
        for (int k = 0; k < 3; ++k) {
            const double ph = 0.7 * k;
            double v = gravity[k] + t1 * a1[k] * std::sin(2 * std::numbers::pi * f1 * t + ph) +
                       t2 * a2[k] * std::sin(2 * std::numbers::pi * f2 * t + 2 * ph) +
                       0.05 * rng.gauss();
            v = std::clamp(v, -16.0, 16.0);               // the sensor saturates
            std::fprintf(f, ",%.4f", v);
        }
        std::fprintf(f, "\n");
    }
    std::fclose(f);
    return true;
}

int main()
{
    if (!write("flightA.csv", 25.0, 1) || !write("flightB.csv", 0.0, 2)) {
        std::printf("cannot write the CSV files\n");
        return 1;
    }
    std::printf("wrote flightA.csv and flightB.csv (2000 rows each: t_s, ax_g, ay_g, az_g)\n");
    return 0;
}
