// F0-57 forensic evidence: Kofi's light-sensor histogram (10-bit readings 0..1023).
// The program reads the simulated log, bins it and prints the table Kofi looked at.
#include <cstdint>
#include <iostream>
#include <random>
#include <vector>

int main()
{
    // Simulated day by a sunny window: most readings spread between 300 and 899;
    // when the sun hits the sensor directly, the reading is stuck at the top code 1023.
    std::mt19937 engine(1023);
    std::vector<int> readings;
    for (int i = 0; i < 2000; ++i) {
        if (engine() % 10 < 3) {
            readings.push_back(1023);
        } else {
            readings.push_back(300 + static_cast<int>(engine() % 600));  // bias of % 600: negligible
        }
    }

    const int lo = 0;
    const int hi = 1023;
    const int bins = 8;
    const int width = (hi - lo) / bins;
    std::vector<int> count(bins, 0);
    for (int r : readings) {
        const int b = (r - lo) / width;
        if (b < bins) {
            ++count[static_cast<std::size_t>(b)];
        }
    }
    std::cout << "readings in the log: " << readings.size() << "\n";
    std::cout << "bin width: " << width << "\n";
    int counted = 0;
    for (int b = 0; b < bins; ++b) {
        std::cout << "bin " << b << "  [" << lo + b * width << ", " << lo + (b + 1) * width
                  << ")  " << count[static_cast<std::size_t>(b)] << "\n";
        counted += count[static_cast<std::size_t>(b)];
    }
    std::cout << "readings in the histogram: " << counted << "\n";
    int maxSeen = 0;
    for (int r : readings) {
        maxSeen = r > maxSeen ? r : maxSeen;
    }
    std::cout << "largest reading in the log: " << maxSeen << "\n";
    return 0;
}
