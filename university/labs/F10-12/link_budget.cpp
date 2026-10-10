// F10-12 Listing 3: a free-space link budget. Received power = transmit power + antenna
// gains - cable losses - free-space path loss, FSPL = 20 log10(4 pi d f / c). Free space
// is the BEST case: obstacles, the ground, the vehicle's own body and interference only
// make it worse. Powers, gains, sensitivity and the three frequencies are exercise values;
// which frequencies and powers you may use is set by your national rules.
#include <cmath>
#include <cstdio>
#include <initializer_list>
#include <numbers>

constexpr double kC = 299792458.0;   // m/s, speed of light (exact by definition)

double fsplDb(double metres, double hz)
{
    return 20.0 * std::log10(4.0 * std::numbers::pi * metres * hz / kC);
}

int main()
{
    const double txDbm = 20.0, gainsDbi = 2.0 + 2.0, cablesDb = 1.0;
    const double sensitivityDbm = -100.0, fadeMarginDb = 10.0;
    std::printf("budget: %.0f dBm + %.0f dBi - %.0f dB cables; receiver needs %.0f dBm; "
                "keep %.0f dB margin\n", txDbm, gainsDbi, cablesDb, sensitivityDbm, fadeMarginDb);
    std::printf("   f MHz  loss 100 m  loss 1 km  rx at 1 km  free-space range with margin\n");
    for (double hz : {433e6, 915e6, 2400e6}) {
        const double rx1km = txDbm + gainsDbi - cablesDb - fsplDb(1000.0, hz);
        // solve FSPL(d) = budget - sensitivity - margin for d
        const double allowed = txDbm + gainsDbi - cablesDb - sensitivityDbm - fadeMarginDb;
        const double d = kC / (4.0 * std::numbers::pi * hz) * std::pow(10.0, allowed / 20.0);
        std::printf("%8.0f %11.1f %10.1f %11.1f %14.1f km\n", hz / 1e6, fsplDb(100.0, hz),
                    fsplDb(1000.0, hz), rx1km, d / 1000.0);
    }
    std::printf("doubling the distance adds %.2f dB of loss at any frequency\n",
                fsplDb(2000.0, 2400e6) - fsplDb(1000.0, 2400e6));
    return 0;
}
