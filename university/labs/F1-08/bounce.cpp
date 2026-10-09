// Forensic evidence generator for "The counter that counts three".
// A slow, noisy signal rises once from 0 V to 5 V (one real event).
// bounce.in holds the voltages sampled once per millisecond (made by a fixed
// formula plus a fixed noise pattern; see the answer key). The circuit counts
// rising edges with ONE threshold at 2.5 V. The output is the evidence.
#include <iomanip>
#include <iostream>
#include <vector>

int main()
{
    std::vector<double> samples;
    double v = 0.0;
    while (std::cin >> v) {
        samples.push_back(v);
    }
    const double threshold = 2.5;
    bool wasHigh = false;
    int count = 0;
    std::cout << std::fixed << std::setprecision(2);
    for (std::size_t i = 0; i < samples.size(); ++i) {
        const bool isHigh = samples[i] > threshold;
        std::cout << "ms " << std::setw(2) << i << "  " << samples[i] << " V  "
                  << (isHigh ? "HIGH" : "low ");
        if (isHigh && !wasHigh) {
            ++count;
            std::cout << "  <- rising edge, count = " << count;
        }
        std::cout << "\n";
        wasHigh = isHigh;
    }
    std::cout << "events counted: " << count << "\n";
    return 0;
}
