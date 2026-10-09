// F1-65 Listing 2: why a gyroscope alone drifts.
// Integrates the z-axis rate of the pretend U-IMU6 (bias 0.10 dps, noise 0.05 dps)
// for 60 s at a pretend 100 samples per second, while the sensor does not turn.
#include "uimu_model.h"

#include <cstdio>

int main()
{
    const double dt = 0.01;          // pretend 100 Hz sample rate
    const double biasEstimate = 0.10; // from a calibration like Listing 1's
    World world;
    Noise noise(777u);
    double rawAngle = 0.0;
    double corrected = 0.0;
    std::printf("%6s %14s %20s\n", "time s", "raw angle deg", "bias-removed deg");
    for (int k = 0; k <= 6000; ++k) {
        if (k % 1000 == 0) {
            std::printf("%6.0f %14.3f %20.3f\n", k * dt, rawAngle, corrected);
        }
        const double rate = world.gyroBiasDps[2] + noise.gaussian(world.gyroNoiseDps);
        rawAngle += rate * dt;
        corrected += (rate - biasEstimate) * dt;
    }
    std::printf("true angle: 0 deg the whole time (the sensor never turned)\n");
    return 0;
}
