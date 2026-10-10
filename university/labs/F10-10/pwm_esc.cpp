// F10-10 Listing 1: pulse-width commands for an ESC. The flight controller sends a pulse
// every frame; the ESC maps the pulse width to throttle using the two end points it
// learned in calibration. All widths, rates and end points are exercise values.
#include <algorithm>
#include <cstdio>
#include <initializer_list>

struct EscCalibration
{
    double lowUs;    // pulse width the ESC treats as zero throttle
    double highUs;   // pulse width the ESC treats as full throttle
};

double escThrottle(double pulseUs, const EscCalibration& c)
{
    return std::clamp((pulseUs - c.lowUs) / (c.highUs - c.lowUs), 0.0, 1.0);
}

int main()
{
    const EscCalibration matched{1000, 2000};   // calibrated with the controller's own range
    const EscCalibration other{1100, 1900};     // calibrated earlier with a different range
    std::printf("1. Same pulses, two calibrations\n");
    std::printf("   command  pulse us   ESC A   ESC B\n");
    for (double cmd : {0.0, 0.05, 0.10, 0.50, 0.90, 0.95, 1.0}) {
        const double pulse = 1000 + 1000 * cmd;
        std::printf("   %6.2f  %8.0f  %6.3f  %6.3f\n", cmd, pulse, escThrottle(pulse, matched),
                    escThrottle(pulse, other));
    }

    std::printf("2. How often can a new pulse be sent?\n");
    std::printf("   frame rate Hz  frame us  longest pulse us  room left us\n");
    for (double hz : {50.0, 400.0, 490.0, 500.0}) {
        const double frame = 1e6 / hz;
        std::printf("   %12.0f  %8.0f  %16.0f  %12.0f%s\n", hz, frame, 2000.0, frame - 2000.0,
                    frame <= 2000.0 ? "   <- no gap left between pulses" : "");
    }

    std::printf("3. Timing resolution: a timer that counts in 1 us steps gives\n");
    std::printf("   %d distinct throttle steps between %0.f and %0.f us\n",
                static_cast<int>(matched.highUs - matched.lowUs), matched.lowUs, matched.highUs);
    return 0;
}
