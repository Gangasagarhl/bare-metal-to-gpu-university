// F1-64 forensic evidence generator: "The 2 Hz ghost".
// A synthetic sensor signal (a steady 1.20 V level) has a 48 Hz interference of
// 0.05 V added, as if a nearby fan motor coupled into the wire. The logger samples
// at 50 Hz with no anti-alias filter. All values are generated here, not measured;
// the 10-bit ADC and 3.0 V reference are pretend exercise values.
#include <cmath>
#include <cstdio>
#include <numbers>
#include <vector>

double signalVolts(double t)
{
    return 1.20 + 0.05 * std::sin(2.0 * std::numbers::pi * 48.0 * t);
}

int adc10(double v)
{
    const int code = static_cast<int>(std::floor(v / 3.0 * 1024));
    return code < 0 ? 0 : (code > 1023 ? 1023 : code);
}

// Prints the first `count` samples, then estimates the frequency of the wobble
// from mean-crossings over a longer stretch of `seconds` of the same log.
void printLog(const char* title, double fs, int count, double seconds)
{
    std::printf("%s (fs = %.0f Hz, %d samples, ADC codes)\n", title, fs, count);
    std::vector<int> codes;
    for (int k = 0; k < count; ++k) {
        codes.push_back(adc10(signalVolts(k / fs)));
    }
    for (int k = 0; k < count; ++k) {
        std::printf("%5d%s", codes[k], (k % 10 == 9) ? "\n" : "");
    }
    int lo = codes[0];
    int hi = codes[0];
    double mean = 0.0;
    for (int c : codes) {
        lo = c < lo ? c : lo;
        hi = c > hi ? c : hi;
        mean += c;
    }
    mean /= count;
    const int total = static_cast<int>(fs * seconds);
    int crossings = 0;
    int prev = adc10(signalVolts(0.0));
    for (int k = 1; k < total; ++k) {
        const int c = adc10(signalVolts(k / fs));
        if ((c >= mean) != (prev >= mean)) {
            ++crossings;
        }
        prev = c;
    }
    std::printf("shown: min %d  max %d  mean %.1f\n", lo, hi, mean);
    std::printf("mean-crossings over %.1f s of this log: %d -> about %.1f Hz\n\n", seconds,
                crossings, crossings / 2.0 / seconds);
}

int main()
{
    printLog("Log A: the robot's normal logger", 50.0, 50, 10.0);
    printLog("Log B: same wire, bench logger", 1000.0, 50, 1.0);
    return 0;
}
