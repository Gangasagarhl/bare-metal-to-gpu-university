// F10-12 forensic evidence generator "The link that fails facing away". A ground range
// check with the propellers removed: the transmitter in its reduced-power check mode at a
// fixed distance, a helper turning the vehicle slowly on a turntable (6 degrees per second).
// The flight controller logs every RC frame it receives: time, heading, the receiver's
// signal strength. SYNTHETIC: the shading model and all levels are exercise values
// described in the answer key. Writes rc_link.csv.
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <numbers>

int main()
{
    std::FILE* f = std::fopen("rc_link.csv", "w");
    if (f == nullptr) {
        std::printf("cannot write rc_link.csv\n");
        return 1;
    }
    std::uint64_t s = 31337;
    auto uniform = [&s]() {
        s = s * 6364136223846793005ULL + 1442695040888963407ULL;
        return static_cast<double>(s >> 11) * 0x1.0p-53;
    };
    std::fprintf(f, "t_ms,heading_deg,rssi_dbm\n");
    int received = 0, slots = 0;
    for (int t = 0; t < 120000; t += 20) {           // 120 s, a frame slot every 20 ms
        ++slots;
        const double heading = std::fmod(t * 0.006, 360.0);
        // the transmitter is due north (heading 0 faces it); shading when facing away
        const double away = std::abs(std::remainder(heading - 190.0, 360.0));
        const double shade =
            away < 40.0 ? 22.0 * std::cos(away / 40.0 * std::numbers::pi / 2) : 0.0;
        const double rssi = -78.0 - shade + 6.0 * (uniform() - 0.5);
        const bool ok = rssi > -96.0 + 4.0 * (uniform() - 0.5);
        if (ok) {
            std::fprintf(f, "%d,%.1f,%.1f\n", t, heading, rssi);
            ++received;
        }
    }
    std::fclose(f);
    std::printf("wrote rc_link.csv: %d frames received out of %d frame slots\n", received, slots);
    return 0;
}
