// Forensic evidence generator for "One bright, one dim".
// Two LEDs are wired in parallel and share ONE series resistor.
// The two LEDs come from different batches: two_leds.in holds their hidden
// pretend curve parameters Is (A) and Vn (V); this is the answer key.
// The output is what the learner gets: the shared voltage and each current.
#include <cmath>
#include <iomanip>
#include <iostream>

int main()
{
    const double supply = 5.0;
    const double ohms = 150.0;
    double isA = 0.0;
    double isB = 0.0;
    double vn = 0.0;
    if (!(std::cin >> isA >> isB >> vn)) {
        return 1;
    }
    auto ledA = [&](double v) { return isA * (std::exp(v / vn) - 1.0); };
    auto ledB = [&](double v) { return isB * (std::exp(v / vn) - 1.0); };

    double low = 0.0;
    double high = supply;
    for (int i = 0; i < 100; ++i) {
        const double mid = 0.5 * (low + high);
        if (ledA(mid) + ledB(mid) > (supply - mid) / ohms) {
            high = mid;
        } else {
            low = mid;
        }
    }
    const double v = 0.5 * (low + high);
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "supply " << supply << " V, shared resistor " << ohms << " ohms\n";
    std::cout << "voltage across both LEDs: " << v << " V\n";
    std::cout << "resistor current: " << (supply - v) / ohms * 1000.0 << " mA\n";
    std::cout << "LED A current:    " << ledA(v) * 1000.0 << " mA\n";
    std::cout << "LED B current:    " << ledB(v) * 1000.0 << " mA\n";
    return 0;
}
