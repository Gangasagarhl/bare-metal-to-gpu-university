// F10-11 Listing 1: an analog power module seen from the flight controller's ADC.
// The module divides the battery voltage down and turns the current into a small voltage;
// the controller converts ADC counts back to volts and amperes with two scale factors and
// integrates current into consumed charge. Divider, sensor gain, offset, ADC width and
// reference are exercise values; a real module's come from its documentation.
#include <cmath>
#include <cstdio>

constexpr double kVref = 3.3;        // V, ADC reference
constexpr int kFull = 4095;          // 12-bit ADC

int adcCounts(double pinVolts)
{
    return static_cast<int>(std::lround(pinVolts / kVref * kFull));
}

double pinVolts(int counts)
{
    return counts * kVref / kFull;
}

int main()
{
    // the module (truth, unknown to the controller)
    const double divider = 11.0;       // battery V / pin V
    const double voltsPerAmp = 0.050;  // current sensor gain
    const double offsetV = 0.010;      // current sensor output at 0 A

    const double vBat = 15.20, iBat = 12.5;
    const int cv = adcCounts(vBat / divider), ci = adcCounts(iBat * voltsPerAmp + offsetV);
    std::printf("1. Battery %.2f V, %.2f A -> ADC counts %d (voltage pin), %d (current pin)\n",
                vBat, iBat, cv, ci);
    std::printf("   one count = %.4f V of battery, %.4f A of current\n",
                pinVolts(1) * divider, pinVolts(1) / voltsPerAmp);

    // 2. the controller's settings: right divider, but a current scale copied from another
    //    module and no offset
    const double setDivider = 11.0, setAmpsPerVolt = 14.0, setOffset = 0.0;
    std::printf("2. With the settings as found: %.3f V, %.3f A\n", pinVolts(cv) * setDivider,
                (pinVolts(ci) - setOffset) * setAmpsPerVolt);

    // 3. two-point calibration against a clamp meter: (counts, metered amperes)
    const int c1 = adcCounts(2.0 * voltsPerAmp + offsetV);
    const int c2 = adcCounts(20.0 * voltsPerAmp + offsetV);
    const double a1 = 2.0, a2 = 20.0;
    const double ampsPerVolt = (a2 - a1) / (pinVolts(c2) - pinVolts(c1));
    const double offset = pinVolts(c1) - a1 / ampsPerVolt;
    std::printf("3. Calibration points: %d counts at %.1f A, %d counts at %.1f A\n", c1, a1, c2,
                a2);
    std::printf("   -> %.3f A per pin volt, offset %.4f V; reading now %.3f A\n", ampsPerVolt,
                offset, (pinVolts(ci) - offset) * ampsPerVolt);

    // 4. consumed charge: integrate 10 minutes at the logged current, 10 samples per second
    double mAhFound = 0, mAhCal = 0;
    for (int k = 0; k < 6000; ++k) {
        mAhFound += (pinVolts(ci) - setOffset) * setAmpsPerVolt * 0.1 / 3.6;
        mAhCal += (pinVolts(ci) - offset) * ampsPerVolt * 0.1 / 3.6;
    }
    std::printf("4. 10 min at %.1f A: true %.0f mAh, settings as found %.0f mAh, "
                "calibrated %.0f mAh\n", iBat, iBat * 600 / 3.6, mAhFound, mAhCal);
    return 0;
}
