// F9-05 Listing 3 (for the curious): what if the sensor reading arrives one step late?
#include <iomanip>
#include <iostream>
#include <vector>

int main()
{
    const std::vector<double> gains = {0.25, 0.5, 1.0};
    std::cout << "gain  crossings in 20 steps  distance after 20 steps\n";
    for (double gain : gains) {
        double previous = 8.0;   // the distance one step ago (what the sensor reports)
        double distance = 8.0;   // the distance now
        int crossings = 0;
        for (int step = 1; step <= 20; ++step) {
            double next = distance - gain * previous;   // correct using the OLD reading
            if ((distance >= 0 && next < 0) || (distance <= 0 && next > 0)) {
                crossings = crossings + 1;
            }
            previous = distance;
            distance = next;
        }
        std::cout << std::fixed << std::setprecision(2) << std::setw(4) << gain
                  << std::setw(14) << crossings << std::setw(25) << distance << '\n';
    }
    return 0;
}
