// reserve.cpp - how much charge does a return home need? Arithmetic for the course
// simulator's vehicle (invented teaching values; measure your own vehicle's currents).
// needed = (climb time + horizontal time + landing time) * current, plus a margin.
#include <cstdio>
#include <initializer_list>

int main()
{
    const double current_a =
        17.5;                   // course simulator, cruise at 5 m/s, mid-pack (sitl_mission.out)
    const double cruise = 5.0;  // m/s, the simulator's horizontal speed limit
    const double rtl_alt = 15.0, cruise_alt = 10.0, climb = 2.0, land = 0.7;  // m, m, m/s, m/s
    const double margin = 0.30;   // 30 % extra for wind estimates, ageing, cold
    const double pack = 1500.0;   // mAh, the F4 pack
    const double low_pct = 30.0;  // the fixed "battery low" threshold
    std::printf("pack %.0f mAh; fixed low threshold %.0f %% = %.0f mAh left at trigger\n", pack,
                low_pct, pack * low_pct / 100.0);
    std::printf("%10s %10s %12s %12s %14s %8s\n", "dist [m]", "headwind", "ground m/s", "time [s]",
                "needed [mAh]", "enough?");
    for (double wind : {0.0, 3.0}) {
        for (double d : {50.0, 200.0, 400.0, 600.0, 800.0}) {
            const double ground = cruise - wind;
            const double t = (rtl_alt - cruise_alt) / climb + d / ground + rtl_alt / land;
            const double mah = t * current_a * 1000.0 / 3600.0 * (1.0 + margin);
            std::printf("%10.0f %10.1f %12.1f %12.1f %14.0f %8s\n", d, wind, ground, t, mah,
                        mah <= pack * low_pct / 100.0 ? "yes" : "NO");
        }
    }
    std::printf("break-even distance, no wind: %.0f m; 3 m/s headwind: %.0f m\n",
                ((pack * low_pct / 100.0) / (1.0 + margin) * 3.6 / current_a -
                 (rtl_alt - cruise_alt) / climb - rtl_alt / land) *
                    cruise,
                ((pack * low_pct / 100.0) / (1.0 + margin) * 3.6 / current_a -
                 (rtl_alt - cruise_alt) / climb - rtl_alt / land) *
                    (cruise - 3.0));
    return 0;
}
