// hover_bug.cpp - forensic evidence for F10-01 ("It hovers low with the camera").
// Height hold of the vertical simulator: thrust = feedforward + kp (setpoint - z) - kd v.
// The vehicle flies twice: before and after a camera was fitted. The log is the evidence.
#include <cmath>
#include <cstdio>

void flight(const char* title, double mass)
{
    const double g = 9.81, k = 0.05, dt = 0.001;
    const double feedforward = 1.0 * g;      // N: hover thrust written into the controller
    const double kp = 4.0, kd = 3.0;          // N per m, N per (m/s)
    double z = 0.0, v = 0.0;
    std::printf("%s (mass %.2f kg)\n", title, mass);
    std::printf("  t(s)  setpoint(m)  z(m)   v(m/s)  thrust(N)\n");
    for (int step = 0; step <= 20000; ++step) {
        const double t = step * dt;
        const double setpoint = 2.0;
        const double thrust = feedforward + kp * (setpoint - z) - kd * v;
        double a = (thrust - mass * g - k * v * std::fabs(v)) / mass;
        if (z <= 0.0 && a < 0.0 && v <= 0.0) {
            a = 0.0;
            v = 0.0;
        }
        if (step % 2000 == 0) {
            std::printf("%6.1f %12.2f %6.3f %7.3f %10.3f\n", t, setpoint, z, v, thrust);
        }
        v += a * dt;
        z += v * dt;
    }
}

int main()
{
    flight("flight A: before the camera", 1.0);
    flight("flight B: with the camera", 1.25);
    return 0;
}
