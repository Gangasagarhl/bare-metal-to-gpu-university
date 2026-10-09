// F0-72 Listing 1: frequency response measured the way a test bench does it. The pretend
// motor's first-order model (tau = 0.19608 s, Kdc = 98.039 rad/s per V) is driven with
// u(t) = sin(w t) volts. After the start-up transient has died out, the output is
// correlated with sin and cos over whole periods to get its amplitude and phase, which
// are compared with |G(jw)| and the angle of G(jw) for G(s) = Kdc / (tau s + 1).
#include <cmath>
#include <complex>
#include <format>
#include <iostream>
#include <numbers>

int main()
{
    const double pi = std::numbers::pi;
    const double tau = 0.19608;
    const double kdc = 98.039;
    std::cout << " w (rad/s) | measured gain   dB     phase | predicted gain   dB     phase\n";
    for (double w : {0.5, 1.0, 5.1, 10.0, 50.0, 100.0}) {
        const double period = 2.0 * pi / w;
        const double dt = period / 20000.0;
        const int settle = static_cast<int>(std::ceil(10.0 * tau / period)) + 2;   // whole periods to wait
        const int measure = 4;                                                       // whole periods to measure
        double y = 0.0;
        double sumSin = 0.0;
        double sumCos = 0.0;
        const int perPeriod = 20000;
        for (int k = 0; k < (settle + measure) * perPeriod; ++k) {
            const double t = k * dt;
            const double u = std::sin(w * t);
            if (k >= settle * perPeriod) {
                sumSin += y * std::sin(w * t) * dt;
                sumCos += y * std::cos(w * t) * dt;
            }
            // exact update of tau y' + y = Kdc u over one small step, holding u constant
            const double decay = std::exp(-dt / tau);
            y = y * decay + kdc * u * (1.0 - decay);
        }
        const double span = measure * period;
        const double a = 2.0 * sumSin / span;   // in-phase part
        const double b = 2.0 * sumCos / span;   // quadrature part
        const double gain = std::hypot(a, b);
        const double phase = std::atan2(b, a) * 180.0 / pi;
        const std::complex<double> g = kdc / (std::complex<double>(1.0, w * tau));
        std::cout << std::format("{:>10.1f} | {:>13.3f} {:>6.1f} {:>8.1f} | {:>14.3f} {:>6.1f} {:>8.1f}\n", w, gain,
                                 20.0 * std::log10(gain), phase, std::abs(g), 20.0 * std::log10(std::abs(g)),
                                 std::arg(g) * 180.0 / pi);
    }
    return 0;
}
