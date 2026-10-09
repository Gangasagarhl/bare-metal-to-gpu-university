// F0-67 number check: recomputes the numbers used in the chapter text.
#include <cmath>
#include <format>
#include <iostream>

int main()
{
    const double m = 2.0;
    const double c = 0.8;
    const double f = 1.6;
    const double tau = m / c;
    const double vInf = f / c;
    std::cout << std::format("tau = {} s, v_inf = {} m/s\n", tau, vInf);
    for (double k : {1.0, 3.0, 5.0}) {
        std::cout << std::format("after {} tau: {:.1f} % of the gap closed\n", k, 100.0 * (1.0 - std::exp(-k)));
    }
    std::cout << std::format("v(2.5) = {:.3f}, v(7.5) = {:.3f}, x(10) = {:.4f} m\n", vInf * (1 - std::exp(-1.0)),
                             vInf * (1 - std::exp(-3.0)), vInf * (10.0 - tau * (1 - std::exp(-4.0))));
    std::cout << std::format("within 1 %: t = tau ln 100 = {:.2f} s\n", tau * std::log(100.0));
    const double fc = 0.6;
    const double v0 = 1.5;
    const double tStop = tau * std::log(1.0 + c * v0 / fc);
    std::cout << std::format("coast: t_stop = {:.4f} s, x_stop = {:.4f} m, viscous-only distance v0 tau = {:.2f} m\n",
                             tStop, (v0 + fc / c) * tau * (1.0 - std::exp(-tStop / tau)) - fc / c * tStop, v0 * tau);
    std::cout << std::format("Q1: tau = {} s, v_inf = {} m/s; lab step 2: v(tau) = {:.3f} m/s\n", 4.0 / 2.0, 6.0 / 2.0,
                             3.0 * (1 - std::exp(-1.0)));
    std::cout << std::format("Q4: {:.2f} m; Q6: t = 120 ln 3 = {:.1f} s; Q7 factor 1 - dt/tau = {}\n", 1.2 * 2.5,
                             120.0 * std::log(3.0), 1.0 - 0.03 / 0.01);
    std::cout << std::format("Q8: doubled worst error about {:.2e} m/s; lab step 4c: {:.2f} m/s\n", 2 * 1.472e-4,
                             (1.6 - 0.6) / 0.8);
    std::cout << std::format("forensic: a at v=0: {:.2f}, v>0: {:.2f}, v<0: {:.2f} m/s^2; creep {:.5f} m/s\n", 0.4 / 2,
                             (0.4 - 0.6) / 2, (0.4 + 0.6) / 2, (0.01497 - 0.00299) / 8.0);
    return 0;
}
