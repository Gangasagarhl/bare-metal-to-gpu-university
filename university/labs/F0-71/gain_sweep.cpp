// F0-71 Listing 2: the same loop simulated in the time domain (no voltage limit) for a
// setpoint step of 100 rad/s, at three gains around the critical gain of Listing 1.
// The ratio of the oscillation size in the second half-second to the first tells
// whether it dies out (< 1), stays (about 1) or grows (> 1).
#include <algorithm>
#include <cmath>
#include <format>
#include <iostream>

double swing(double kp, double& ratio)
{
    const double tm = 0.19608;
    const double ta = 0.01;
    const double tf = 0.02;
    const double kdc = 98.039;
    const double dt = 1e-5;
    double u = 0.0;       // driver output (V), lags the command
    double w = 0.0;       // motor speed (rad/s)
    double y = 0.0;       // filtered speed measurement
    double lo1 = 1e300;
    double hi1 = -1e300;
    double lo2 = 1e300;
    double hi2 = -1e300;
    const int n = static_cast<int>(std::lround(1.5 / dt));
    for (int k = 0; k < n; ++k) {
        const double t = k * dt;
        const double command = kp * (100.0 - y);
        u += (command - u) / ta * dt;
        w += (kdc * u - w) / tm * dt;
        y += (w - y) / tf * dt;
        if (t >= 0.5 && t < 1.0) {
            lo1 = std::min(lo1, w);
            hi1 = std::max(hi1, w);
        } else if (t >= 1.0) {
            lo2 = std::min(lo2, w);
            hi2 = std::max(hi2, w);
        }
    }
    ratio = (hi2 - lo2) / (hi1 - lo1);
    return hi2 - lo2;
}

int main()
{
    std::cout << "    Kp   peak-to-peak swing 1.0-1.5 s (rad/s)   ratio to 0.5-1.0 s   behaviour\n";
    for (double kp : {0.2, 0.3475, 0.5}) {
        double ratio = 0.0;
        const double s = swing(kp, ratio);
        const char* what = ratio < 0.9 ? "dies out" : (ratio > 1.1 ? "grows" : "keeps going");
        std::cout << std::format("{:>6.4f} {:>40.4f} {:>20.4f}   {}\n", kp, s, ratio, what);
    }
    return 0;
}
