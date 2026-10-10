// The fix for "The counter that counts three": count with TWO thresholds
// (hysteresis). The signal must rise above 3.0 V to count as HIGH and must
// fall below 2.0 V before it can count as LOW again. Same samples as bounce.in.
#include <iostream>
#include <vector>

int main()
{
    std::vector<double> samples;
    double v = 0.0;
    while (std::cin >> v) {
        samples.push_back(v);
    }
    const double upper = 3.0;
    const double lower = 2.0;
    bool high = false;
    int count = 0;
    for (const double s : samples) {
        if (!high && s > upper) {
            high = true;
            ++count;
        } else if (high && s < lower) {
            high = false;
        }
    }
    std::cout << "samples: " << samples.size() << ", events counted with hysteresis: " << count
              << "\n";
    return 0;
}
