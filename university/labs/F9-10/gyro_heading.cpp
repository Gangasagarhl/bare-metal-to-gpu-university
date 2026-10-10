// F9-10 Listing 2: heading from a z-axis gyro, integrated at 100 Hz for 120 s.
// The robot stands still for 2 s, turns left 90 degrees, drives, turns back, then parks.
// The simulated gyro has a constant bias plus deterministic pseudo-random noise.
// Bias and noise are PRETEND values; real ones come from the IMU's datasheet and from
// your own measurement (F1-65).
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <numbers>

// Small deterministic noise source (xorshift), so every run prints the same numbers.
struct Noise
{
    std::uint32_t s = 2463534242u;
    double next()  // roughly uniform in [-1, 1]
    {
        s ^= s << 13;
        s ^= s >> 17;
        s ^= s << 5;
        return static_cast<double>(s) / 2147483647.5 - 1.0;
    }
};

double trueRate(double t)  // rad/s, the robot's real turning rate
{
    const double r = std::numbers::pi / 2.0 / 3.0;  // 90 degrees in 3 s
    if (t >= 10.0 && t < 13.0) return r;
    if (t >= 40.0 && t < 43.0) return -r;
    return 0.0;
}

int main()
{
    const double dt = 0.01;
    const double bias = 0.0040;      // rad/s
    const double noiseAmp = 0.0100;  // rad/s
    Noise noise;
    double headRaw = 0.0;            // integrated without bias correction
    double headCal = 0.0;            // integrated after subtracting the start-up bias estimate
    double truth = 0.0;
    double sum = 0.0;
    int n = 0;
    double biasEst = 0.0;
    const double deg = 180.0 / std::numbers::pi;
    std::printf("  t_s   true_deg   raw_deg   calibrated_deg\n");
    for (int k = 0; k < 12000; ++k) {
        const double t = k * dt;
        const double meas = trueRate(t) + bias + noiseAmp * noise.next();
        if (t < 2.0) {  // robot known to be still: average the gyro to estimate the bias
            sum += meas;
            ++n;
            biasEst = sum / n;
        }
        truth += trueRate(t) * dt;
        headRaw += meas * dt;
        headCal += (meas - biasEst) * dt;
        if (k % 1500 == 0 || k == 11999) {
            std::printf("%6.2f %9.2f %9.2f %13.2f\n", t + dt, truth * deg, headRaw * deg,
                        headCal * deg);
        }
    }
    std::printf("bias estimate from the first 2 s: %.5f rad/s (true bias %.5f rad/s)\n", biasEst,
                bias);
    return 0;
}
