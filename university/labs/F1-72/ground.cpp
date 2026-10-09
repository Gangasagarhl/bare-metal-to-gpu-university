// F1-72 forensic evidence generator: "The sensor that reads high when the motors run".
// SYNTHETIC log from the university's wiring model. An analog distance sensor puts
// out a steady 1.000 V relative to ITS ground pin. Its ground wire joins the motor
// ground wire, and the last 0.05 ohm (pretend) back to the battery is shared. The
// ADC measures against the MCU's ground at the battery, so it sees
// V = 1.000 + (motor current) x 0.05 + small noise.
// The learner gets this log only; the wiring detail above is in the answer key.
#include "noise.h"

#include <cstdio>

int main()
{
    Noise noise(7u);
    const double rShared = 0.05;  // ohm, pretend
    std::printf("logged every 0.1 s (logger samples at 100 Hz; 1 row in 10 shown)\n");
    std::printf("%6s %10s %12s %10s\n", "t s", "motor cmd", "motor A", "sensor V");
    for (int k = 0; k < 400; ++k) {
        const double t = k * 0.01;
        double cmd = 0.0;
        if (t >= 1.0 && t < 2.0) {
            cmd = 0.5;
        } else if (t >= 2.0 && t < 3.0) {
            cmd = 1.0;
        }
        const double amps = cmd * 3.0 + (cmd > 0.0 ? noise.gaussian(0.15) : 0.0);
        const double v = 1.000 + amps * rShared + noise.gaussian(0.002);
        if (k % 10 == 0) {
            std::printf("%6.1f %10.1f %12.2f %10.4f\n", t, cmd, amps, v);
        }
    }
    std::printf("note: the robot stood still against a wall 0.40 m away for the whole log\n");
    return 0;
}
