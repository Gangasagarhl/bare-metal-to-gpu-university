// F0-59 forensic evidence: the fridge-temperature alarm that rings all day.
// Calibration found mean 4.0 C and standard deviation 0.5 C for the noisy readings of a
// healthy fridge (exercise values). The alarm should ring when a reading is "3 sigma" high.
#include <cmath>
#include <iostream>
#include <random>

double uniform01(std::mt19937& engine)
{
    return (static_cast<double>(engine()) + 0.5) / 4294967296.0;
}

double standardGaussian(std::mt19937& engine)
{
    const double u1 = uniform01(engine);
    const double u2 = uniform01(engine);
    return std::sqrt(-2.0 * std::log(u1)) * std::cos(2.0 * std::acos(-1.0) * u2);
}

int main()
{
    const double calibratedMean = 4.0;
    const double calibratedVariance = 0.25;
    const double threshold = calibratedMean + 3.0 * calibratedVariance;

    std::mt19937 engine(4);
    const int readings = 10000;   // one reading every few seconds, one healthy day
    int alarms = 0;
    double highest = 0.0;
    for (int i = 0; i < readings; ++i) {
        const double t = 4.0 + 0.5 * standardGaussian(engine);  // healthy fridge
        if (t > threshold) {
            ++alarms;
        }
        highest = std::fmax(highest, t);
    }
    std::cout << "calibration: mean " << calibratedMean << " C, variance " << calibratedVariance
              << "\n";
    std::cout << "alarm threshold: " << threshold << " C\n";
    std::cout << "readings today: " << readings << ", alarms: " << alarms << " ("
              << 100.0 * alarms / readings << " %)\n";
    std::cout << "highest reading today: " << highest << " C\n";
    return 0;
}
