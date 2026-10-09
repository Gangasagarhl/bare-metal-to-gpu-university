// F0-67 forensic evidence generator: a cart simulator used to check whether a small
// push can move a parked cart. The friction model contains the fault described only in
// the answer key. Static friction of this cart is 0.6 N; the push is 0.4 N.
#include <format>
#include <iostream>

double sign(double v)
{
    return (v > 0.0) ? 1.0 : ((v < 0.0) ? -1.0 : 0.0);
}

int main()
{
    const double m = 2.0;
    const double fc = 0.6;      // N
    const double push = 0.4;    // N, less than fc
    const double dt = 0.01;
    double v = 0.0;
    double x = 0.0;
    std::cout << "  t (s)   speed (m/s)   position (m)\n";
    for (int k = 1; k <= 1000; ++k) {
        const double a = (push - fc * sign(v)) / m;
        x += v * dt;
        v += a * dt;
        if (k <= 6 || k % 200 == 0) {
            std::cout << std::format("{:>7.2f} {:>13.5f} {:>14.5f}\n", k * dt, v, x);
        }
    }
    return 0;
}
