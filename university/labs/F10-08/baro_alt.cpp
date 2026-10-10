// F10-08 Listing 1: from barometric pressure to height, with the isothermal
// (constant-temperature) hydrostatic model h = (R*T / (M*g)) * ln(p0 / p).
// The physical constants are the values typed below; check them against the
// sources named in the chapter before reusing them.
#include <cmath>
#include <cstdio>
#include <initializer_list>

constexpr double R = 8.314462618;   // J/(mol K), molar gas constant
constexpr double M = 0.0289644;     // kg/mol, molar mass of dry air
constexpr double g = 9.80665;       // m/s^2

double heightAbove(double p0, double p, double tempC)
{
    const double T = tempC + 273.15;
    return R * T / (M * g) * std::log(p0 / p);
}

int main()
{
    const double p0 = 100000.0;   // Pa at the take-off point, stored when the vehicle arms
    std::printf("1. Height for pressure drops below p0 = %.0f Pa (air at 15 C)\n", p0);
    for (double dp : {1.0, 10.0, 100.0, 1000.0}) {
        std::printf("   p0 - %6.0f Pa -> %8.3f m\n", dp, heightAbove(p0, p0 - dp, 15.0));
    }

    const double sens = heightAbove(p0, p0 - 1.0, 15.0);
    std::printf("2. Near p0, 1 Pa is %.4f m; pressure noise of 2 Pa (1 sigma) "
                "is about %.2f m of height noise\n", sens, 2.0 * sens);

    std::printf("3. Same 1000 Pa drop, different assumed air temperature\n");
    for (double t : {-10.0, 15.0, 35.0}) {
        std::printf("   %5.1f C -> %7.2f m\n", t, heightAbove(p0, p0 - 1000.0, t));
    }

    std::printf("4. The weather lowers the ground pressure by 100 Pa during a session;\n"
                "   the vehicle sits on the ground with the p0 stored at arming: reads %+.2f m\n",
                heightAbove(p0, p0 - 100.0, 15.0));
    return 0;
}
