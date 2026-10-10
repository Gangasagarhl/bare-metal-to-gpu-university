// F10-10 forensic evidence generator "One motor lags". A bench motor test with the
// propellers removed: the ground station steps each motor's command from 0 to 100 %
// and the ESCs report speed telemetry. SYNTHETIC: the ESC model, end points, motor
// speed constant and battery voltage are exercise values; the fault is in the key.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>

struct Esc
{
    double lowUs, highUs;     // learned end points
};

int main()
{
    const std::array<Esc, 4> esc{{{1000, 2000}, {1000, 2000}, {1100, 1900}, {1000, 2000}}};
    const double rpmPerVolt = 900.0, volts = 14.8;   // no-load model (exercise values)
    std::uint64_t s = 99;
    auto noise = [&s]() {
        s = s * 6364136223846793005ULL + 1442695040888963407ULL;
        return static_cast<double>(s >> 11) * 0x1.0p-53 - 0.5;   // -0.5 .. 0.5
    };
    std::printf("bench motor test, propellers removed; speed telemetry in rpm\n");
    std::printf("command %%  pulse us   motor 1  motor 2  motor 3  motor 4\n");
    for (int pct = 0; pct <= 100; pct += 10) {
        const double pulse = 1000 + 10.0 * pct;
        std::printf("%9d  %8.0f", pct, pulse);
        for (const auto& e : esc) {
            const double thr = std::clamp((pulse - e.lowUs) / (e.highUs - e.lowUs), 0.0, 1.0);
            const double rpm = thr < 0.04 ? 0.0 : thr * rpmPerVolt * volts + 40.0 * noise();
            std::printf("  %7.0f", std::max(0.0, rpm));
        }
        std::printf("\n");
    }
    return 0;
}
