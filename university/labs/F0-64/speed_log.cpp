// F0-64 forensic evidence generator: a cart logger that turns distance samples into
// speeds. It contains deliberate faults (described only in the answer key).
#include <format>
#include <iostream>
#include <vector>

struct Sample
{
    double timeS;      // time stamp in seconds
    double distanceMm; // distance travelled in millimetres
};

int main()
{
    // Samples every 0.1 s from a cart moving at 0.85 m/s; the 5th time stamp was
    // written twice by the logger.
    std::vector<Sample> log;
    for (int k = 0; k <= 8; ++k) {
        const double t = 0.1 * k;
        log.push_back({t, 850.0 * t});
    }
    log[5].timeS = log[4].timeS;
    std::cout << "sample  time (s)  distance (mm)  speed shown on dashboard (m/s)\n";
    for (std::size_t k = 1; k < log.size(); ++k) {
        const double dt = log[k].timeS - log[k - 1].timeS;
        const double speed = (log[k].distanceMm - log[k - 1].distanceMm) / dt;
        std::cout << std::format("{:>6} {:>9.2f} {:>14.1f}  {:>12.1f}\n", k, log[k].timeS, log[k].distanceMm, speed);
    }
    return 0;
}
