// F0-66 Listing 3: the running sum (discrete integral) and the low-pass filter as
// they appear inside a controller loop. Error e[k] = 1 for 0.5 s, then 0.
#include <format>
#include <iostream>

int main()
{
    const double dt = 0.01;
    const double tau = 0.1;
    const double alpha = dt / (tau + dt);
    double integral = 0.0;
    double filtered = 0.0;
    std::cout << "  t (s)   e    integral of e   low-pass of e\n";
    for (int k = 0; k <= 100; ++k) {
        const double t = k * dt;
        const double e = (k < 50) ? 1.0 : 0.0;
        integral += e * dt;                  // rectangle rule, as most controllers do
        filtered += alpha * (e - filtered);  // first-order low-pass, time constant tau
        if (k % 10 == 0) {
            std::cout << std::format("{:>7.2f} {:>4.0f} {:>15.3f} {:>15.3f}\n", t, e, integral, filtered);
        }
    }
    std::cout << std::format("alpha = dt / (tau + dt) = {:.6f}\n", alpha);
    return 0;
}
