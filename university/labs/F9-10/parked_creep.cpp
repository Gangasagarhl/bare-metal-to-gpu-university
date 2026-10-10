// F9-10 forensic generator: the robot estimates its gyro bias during the first 2 s
// after power-on, then parks for 10 minutes. Two boots are logged. What differs
// between the boots is described in the answer key only. ALL VALUES ARE PRETEND.
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <numbers>

struct Noise
{
    std::uint32_t s;
    double next()
    {
        s ^= s << 13;
        s ^= s >> 17;
        s ^= s << 5;
        return static_cast<double>(s) / 2147483647.5 - 1.0;
    }
};

void boot(const char* name, std::uint32_t seed, double handTurnRate)
{
    const double dt = 0.01;
    const double bias = 0.0040;
    const double noiseAmp = 0.0100;
    const double deg = 180.0 / std::numbers::pi;
    Noise noise{seed};
    double sum = 0.0;
    double sumSq = 0.0;
    int n = 0;
    // Calibration window: 2 s. In one boot the robot is being turned by hand on the floor
    // for the first 1.5 s of the window (true rate handTurnRate), then left alone.
    for (int k = 0; k < 200; ++k) {
        const double truth = (k < 150) ? handTurnRate : 0.0;
        const double m = truth + bias + noiseAmp * noise.next();
        sum += m;
        sumSq += m * m;
        ++n;
    }
    const double mean = sum / n;
    const double sd = std::sqrt(sumSq / n - mean * mean);
    std::printf("%s: calibration window 2.00 s, %d samples: mean %.5f rad/s, std dev %.5f rad/s\n",
                name, n, mean, sd);
    std::printf("%s: parked; heading in degrees after", name);
    double heading = 0.0;
    for (int k = 1; k <= 60000; ++k) {
        const double m = bias + noiseAmp * noise.next();
        heading += (m - mean) * dt;
        if (k == 1000 || k == 6000 || k == 12000 || k == 30000 || k == 60000) {
            std::printf("  %d s: %.1f", k / 100, heading * deg);
        }
    }
    std::printf("\n");
}

int main()
{
    boot("boot 1 (Monday)", 2463534242u, 0.0);
    boot("boot 2 (Tuesday)", 88172645u, 0.10);
    return 0;
}
