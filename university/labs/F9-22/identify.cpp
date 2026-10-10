// F9-22 Listing 3: estimate the motor's static gain and time constant from a step log
// (read from standard input: a header line, then "time speed" pairs), and propose
// PI gains with the F9-18 rule: Ki = Kp / tau, Kp chosen for a target closed-loop time.
#include <cmath>
#include <format>
#include <iostream>
#include <string>
#include <vector>

int main()
{
    std::string word;
    double duty = 0.0;
    double period = 0.0;
    double countsPerRev = 0.0;
    std::cin >> word >> duty >> word >> period >> word >> countsPerRev;
    std::vector<double> times;
    std::vector<double> speeds;
    double t = 0.0;
    double w = 0.0;
    while (std::cin >> t >> w) {
        times.push_back(t);
        speeds.push_back(w);
    }
    if (speeds.size() < 20) {
        std::cout << "not enough samples\n";
        return 1;
    }
    double finalSpeed = 0.0; // mean of the last 20 samples smooths the encoder steps
    for (std::size_t i = speeds.size() - 20; i < speeds.size(); ++i) {
        finalSpeed += speeds[i] / 20.0;
    }
    double tau = -1.0;
    for (std::size_t i = 0; i < speeds.size(); ++i) {
        if (speeds[i] >= 0.632 * finalSpeed) {
            tau =
                times[i] - period / 2.0; // a speed from counts belongs to the middle of its period
            break;
        }
    }
    const double volts = duty * 12.0;
    const double gainPerVolt = finalSpeed / volts;
    std::cout << std::format("samples {}, final speed {:.1f} rad/s at {:.2f} V\n", speeds.size(),
                             finalSpeed, volts);
    std::cout << std::format(
        "static gain {:.1f} rad/s per V (includes the deadband), tau {:.3f} s\n", gainPerVolt, tau);
    const double targetTime = 0.05; // wanted closed-loop time constant, s
    const double kp = tau / (gainPerVolt * targetTime);
    const double ki = kp / tau;
    std::cout << std::format(
        "PI proposal for a {:.2f} s closed loop: Kp = {:.4f} V per rad/s, Ki = {:.4f} V per rad\n",
        targetTime, kp, ki);
    return 0;
}
