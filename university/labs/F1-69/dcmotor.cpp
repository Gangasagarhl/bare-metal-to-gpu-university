// F1-69 Listing 1: the university's brushed DC motor model.
// Electrical:  V = R i + L di/dt + Ke w      (w = shaft speed, rad/s)
// Mechanical:  J dw/dt = Kt i - b w - T_load
// Integrated with small Euler steps. ALL PARAMETERS ARE PRETEND EXERCISE VALUES;
// a real motor's values come from its datasheet or from measuring it.
#include <cstdio>
#include <initializer_list>
#include <numbers>

struct Motor
{
    double r = 2.0;     // ohm
    double l = 1.0e-3;  // henry
    double ke = 0.01;   // V per rad/s
    double kt = 0.01;   // N m per A (equal to ke in SI units)
    double j = 1.0e-5;  // kg m^2 (rotor plus wheel)
    double b = 1.0e-6;  // N m per rad/s (viscous friction)
    double i = 0.0;     // A
    double w = 0.0;     // rad/s

    void step(double v, double load, double dt)
    {
        const double di = (v - r * i - ke * w) / l;
        double dw = (kt * i - b * w - load) / j;
        if (w <= 0.0 && dw < 0.0) {
            dw = 0.0;  // load friction cannot turn the shaft backwards
        }
        i += di * dt;
        w += dw * dt;
    }
};

double rpm(double w)
{
    return w * 60.0 / (2.0 * std::numbers::pi);
}

int main()
{
    const double dt = 1.0e-5;
    Motor m;
    std::printf("Part A: 6 V step, no load\n%8s %10s %10s\n", "t ms", "current A", "speed rpm");
    for (int k = 0; k <= 100000; ++k) {
        const int ms10 = k / 1000;
        if (k % 1000 == 0 && (ms10 <= 2 || ms10 % 20 == 0)) {
            std::printf("%8.0f %10.3f %10.0f\n", k * dt * 1000.0, m.i, rpm(m.w));
        }
        m.step(6.0, 0.0, dt);
    }
    std::printf("\nPart B: steady state after 1 s, no load\n");
    for (double v : {2.0, 4.0, 6.0}) {
        Motor x;
        for (int k = 0; k < 100000; ++k) {
            x.step(v, 0.0, dt);
        }
        std::printf("  %.0f V -> %6.0f rpm, %.3f A\n", v, rpm(x.w), x.i);
    }
    std::printf("\nPart C: torque-speed line at 6 V (steady state after 1 s)\n");
    std::printf("  stall current V/R = %.2f A, stall torque Kt*V/R = %.4f N m\n", 6.0 / m.r,
                m.kt * 6.0 / m.r);
    for (double load : {0.0, 0.0075, 0.015, 0.0225, 0.029}) {
        Motor x;
        for (int k = 0; k < 100000; ++k) {
            x.step(6.0, load, dt);
        }
        std::printf("  load %.4f N m -> %6.0f rpm, %.3f A, mechanical power %.3f W\n", load,
                    rpm(x.w), x.i, load * x.w);
    }
    return 0;
}
