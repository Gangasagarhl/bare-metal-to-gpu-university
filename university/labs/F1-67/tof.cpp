// F1-67 Listing 1: time of flight. Light goes out and back, so
// distance = c * t / 2. The speed of light in vacuum c = 299792458 m/s exactly
// (it is fixed by the SI definition of the metre). Timer resolutions below are
// pretend exercise values, not the numbers of any real sensor.
#include <cstdio>
#include <initializer_list>

int main()
{
    const double c = 299792458.0;  // m/s, exact by definition
    std::printf("round-trip time for a target at distance d (t = 2d / c)\n");
    for (double d : {0.10, 1.0, 10.0, 100.0}) {
        const double t = 2.0 * d / c;
        std::printf("  d = %7.2f m  ->  t = %10.3f ns\n", d, t * 1e9);
    }
    std::printf("\nrange step for a timer that counts in ticks of dt (step = c * dt / 2)\n");
    for (double dtPs : {1000.0, 100.0, 10.0}) {
        const double step = c * dtPs * 1e-12 / 2.0;
        std::printf("  dt = %6.0f ps  ->  range step = %8.2f mm\n", dtPs, step * 1000.0);
    }
    std::printf("\nconverting measured ticks to distance (tick = 100 ps, pretend)\n");
    for (long ticks : {0L, 1L, 67L, 667L, 6671L}) {
        std::printf("  %5ld ticks -> %8.4f m\n", ticks, c * ticks * 100e-12 / 2.0);
    }
    return 0;
}
