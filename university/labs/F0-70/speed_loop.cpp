// F0-70 Listing 2: proportional speed control of the pretend motor (full model with L).
// Prediction from the transfer functions (L neglected):
//   T(s) = Kp Kdc / (tau_m s + 1 + Kp Kdc):  final speed = r Kp Kdc / (1 + Kp Kdc),
//   closed-loop time constant = tau_m / (1 + Kp Kdc).
// The simulation reports the final speed and the time to reach 63.2 % of it.
#include <algorithm>
#include <cmath>
#include <format>
#include <iostream>

int main()
{
    const double r = 2.0;
    const double l = 1e-3;
    const double k = 0.01;
    const double j = 1e-5;
    const double b = 1e-6;
    const double tauM = j * r / (k * k + r * b);
    const double kdc = k / (k * k + r * b);
    const double setpoint = 300.0;   // rad/s
    const double dt = 1e-6;
    std::cout << "   Kp   | predicted: final   error   tau      | simulated: final   tau      | max volts\n";
    for (double kp : {0.01, 0.05, 0.2}) {
        const double loop = kp * kdc;
        const double finalPred = setpoint * loop / (1.0 + loop);
        const double tauPred = tauM / (1.0 + loop);
        double i = 0.0;
        double w = 0.0;
        double t63 = -1.0;
        double vmax = 0.0;
        const int n = static_cast<int>(std::lround(2.0 / dt));
        for (int s = 1; s <= n; ++s) {
            const double volts = kp * (setpoint - w);    // the controller: V = Kp (r - w)
            vmax = std::max(vmax, volts);
            const double di = (volts - r * i - k * w) / l;
            const double dw = (k * i - b * w) / j;
            i += di * dt;
            w += dw * dt;
            if (t63 < 0.0 && w >= 0.632 * finalPred) {
                t63 = s * dt;
            }
        }
        std::cout << std::format("{:>7.2f} | {:>16.2f} {:>7.2f} {:>7.4f} s | {:>16.2f} {:>7.4f} s | {:>7.2f}\n", kp,
                                 finalPred, setpoint - finalPred, tauPred, w, t63, vmax);
    }
    return 0;
}
