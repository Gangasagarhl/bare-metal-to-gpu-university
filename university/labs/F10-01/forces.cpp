// forces.cpp - the four forces on a hovering or climbing multirotor, in numbers (F10-01).
// Exercise values of the course quad: mass 1.0 kg, four rotors of at most 10 N each.
#include <cmath>
#include <cstdio>
#include <initializer_list>
#include <numbers>

int main()
{
    const double g = 9.81;          // m/s^2
    const double maxRotor = 10.0;   // N: the most one rotor can push (exercise value)
    const double maxThrust = 4 * maxRotor;

    std::printf("mass(kg)  weight(N)  hover thrust per rotor(N)  thrust-to-weight\n");
    for (double mass : {0.5, 1.0, 2.0, 4.0, 4.5}) {
        const double weight = mass * g;
        std::printf("%8.2f %10.2f %26.3f %17.2f%s\n", mass, weight, weight / 4.0,
                    maxThrust / weight, maxThrust >= weight ? "" : "   <- cannot hover");
    }

    std::printf("\nTilted by angle a, total thrust T = m g / cos(a) keeps the height;\n");
    std::printf("its sideways part gives the acceleration g tan(a).\n");
    std::printf("tilt(deg)  thrust needed(N)  sideways accel(m/s^2)\n");
    const double mass = 1.0;
    for (double deg : {0.0, 10.0, 20.0, 30.0, 45.0, 60.0, 75.0, 80.0}) {
        const double a = deg * std::numbers::pi / 180.0;
        const double thrust = mass * g / std::cos(a);
        std::printf("%9.0f %17.3f %22.3f%s\n", deg, thrust, g * std::tan(a),
                    thrust > maxThrust ? "   <- more than the rotors can give" : "");
    }
    return 0;
}
