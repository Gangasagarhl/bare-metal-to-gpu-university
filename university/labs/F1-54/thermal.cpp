// thermal.cpp - F1-54 Listing 2: a toy chip that heats up, and a throttling rule that
// lowers its clock (and voltage) to stay under a temperature limit.
// Every number is in made-up "toy units"; nothing here describes a real processor.
#include <cstdio>

struct Step
{
    double freq;      // relative clock frequency
    double volt;      // relative supply voltage needed at that frequency
};

int main()
{
    const Step steps[] = {{1.0, 1.0}, {0.8, 0.9}, {0.6, 0.8}, {0.5, 0.75}};
    const double ambient = 25.0;     // toy degrees
    const double limit = 90.0;       // throttle above this
    const double resume = 85.0;      // speed up again below this
    const double heatPerPower = 80.0;
    const double smoothing = 0.05;   // how fast the toy chip's temperature follows

    // Dynamic power grows with C * V^2 * f; C is folded into the constant 1.0 here.
    std::printf("level  freq  volt  dynamic power V^2*f\n");
    for (int i = 0; i < 4; ++i) {
        const Step s = steps[i];
        std::printf("%5d %5.2f %5.2f %10.3f\n", i, s.freq, s.volt, s.volt * s.volt * s.freq);
    }

    int level = 0;
    double temp = ambient;
    double work = 0.0;
    std::printf("\ntime  level  freq  temp  work done\n");
    for (int t = 0; t <= 200; ++t) {
        const Step s = steps[level];
        const double power = 0.1 + s.volt * s.volt * s.freq;   // 0.1: leakage (static) power
        temp += (ambient + heatPerPower * power - temp) * smoothing;
        work += s.freq;
        if (temp > limit && level < 3) {
            ++level;
        } else if (temp < resume && level > 0) {
            --level;
        }
        if (t % 20 == 0) {
            std::printf("%4d %6d %5.2f %5.1f %8.1f\n", t, level, steps[level].freq, temp, work);
        }
    }
    std::printf("average speed over the run: %.2f of full speed\n", work / 201.0);
    return 0;
}
