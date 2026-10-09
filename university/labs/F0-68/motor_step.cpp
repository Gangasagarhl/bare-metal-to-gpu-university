// F0-68 Listing 1 (course lab): step response of a motor model.
// The pretend brushed DC motor of F1-69 (R = 2 ohm, L = 1 mH, K = 0.01, J = 1e-5 kg m^2,
// b = 1e-6 N m s) gets a 6 V step. We simulate the full model
//   L di/dt = V - R i - K w,   J dw/dt = K i - b w
// and compare it with the first-order approximation (L neglected):
//   tau_m dw/dt + w = Kdc V,   tau_m = J R / (K^2 + R b),   Kdc = K / (K^2 + R b).
#include <cmath>
#include <format>
#include <iostream>

struct Motor
{
    double r = 2.0;
    double l = 1e-3;
    double k = 0.01;
    double j = 1e-5;
    double b = 1e-6;
    double i = 0.0;   // A
    double w = 0.0;   // rad/s
};

void step(Motor& m, double volts, double dt)
{
    const double di = (volts - m.r * m.i - m.k * m.w) / m.l;
    const double dw = (m.k * m.i - m.b * m.w) / m.j;
    m.i += di * dt;
    m.w += dw * dt;
}

int main()
{
    Motor m;
    const double volts = 6.0;
    const double tauM = m.j * m.r / (m.k * m.k + m.r * m.b);
    const double kdc = m.k / (m.k * m.k + m.r * m.b);
    const double tauE = m.l / m.r;
    const double wInf = kdc * volts;
    std::cout << std::format("tau_m = {:.4f} s, tau_e = L/R = {:.4f} s, Kdc = {:.3f} rad/s per V\n", tauM, tauE, kdc);
    std::cout << std::format("final speed Kdc * 6 V = {:.2f} rad/s = {:.0f} rpm\n", wInf,
                             wInf * 60.0 / (2.0 * 3.141592653589793));
    std::cout << "  t/tau_m   t (s)   w full (rad/s)   w 1st order   % of final   current (A)\n";
    const double dt = 1e-6;
    double t = 0.0;
    const double marks[] = {0.0, 0.5, 1.0, 2.0, 3.0, 4.0, 5.0};
    int next = 0;
    while (next < 7) {
        if (t >= marks[next] * tauM - dt / 2.0) {
            const double first = wInf * (1.0 - std::exp(-t / tauM));
            std::cout << std::format("{:>9.1f} {:>7.4f} {:>16.2f} {:>13.2f} {:>12.1f} {:>13.3f}\n", marks[next], t, m.w,
                                     first, 100.0 * m.w / wInf, m.i);
            ++next;
        }
        step(m, volts, dt);
        t += dt;
    }
    return 0;
}
