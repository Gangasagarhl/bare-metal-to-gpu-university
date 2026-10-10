// F1-69 Listing 2: a hobby-servo model. A servo is a motor, a gearbox, a position
// sensor and a small controller in one box. The command is a pulse width.
// PRETEND mapping: 1.0 ms -> -90 deg, 1.5 ms -> 0 deg, 2.0 ms -> +90 deg; pretend
// top speed 300 deg/s; pretend deadband 1 deg. A real servo's mapping, speed and
// pulse timing come from its datasheet.
#include <cmath>
#include <cstdio>

double targetFromPulse(double ms)
{
    if (ms < 1.0) {
        ms = 1.0;
    }
    if (ms > 2.0) {
        ms = 2.0;
    }
    return (ms - 1.5) * 180.0;  // degrees
}

int main()
{
    const double dt = 0.001;
    const double maxSpeed = 300.0;  // deg/s
    const double gain = 20.0;       // 1/s: speed command = gain * error
    const double deadband = 1.0;    // deg
    double angle = 0.0;
    std::printf("%6s %9s %9s %9s\n", "t s", "pulse ms", "target", "angle");
    for (int k = 0; k <= 1500; ++k) {
        const double t = k * dt;
        const double pulse = t < 0.1 ? 1.5 : (t < 0.8 ? 2.0 : 1.25);
        const double target = targetFromPulse(pulse);
        const double err = target - angle;
        double speed = std::fabs(err) < deadband ? 0.0 : gain * err;
        if (speed > maxSpeed) {
            speed = maxSpeed;
        }
        if (speed < -maxSpeed) {
            speed = -maxSpeed;
        }
        if (k % 100 == 0) {
            std::printf("%6.2f %9.2f %9.1f %9.1f\n", t, pulse, target, angle);
        }
        angle += speed * dt;
    }
    return 0;
}
