// F9-05 Listing 2: try many gains and count the zig-zags.
#include <iomanip>
#include <iostream>
#include <vector>

int main()
{
    const std::vector<double> gains = {0.2, 0.5, 1.0, 1.5, 1.8, 2.0, 2.2};
    std::cout << "gain  crossings  distance after 12 steps\n";
    for (double gain : gains) {
        double distance = 8.0;
        int crossings = 0;                       // times the robot jumped over the line
        for (int step = 1; step <= 12; ++step) {
            double next = distance - gain * distance;
            if ((distance >= 0 && next < 0) || (distance <= 0 && next > 0)) {
                crossings = crossings + 1;
            }
            distance = next;
        }
        std::cout << std::fixed << std::setprecision(1) << std::setw(4) << gain
                  << std::setw(11) << crossings << std::setw(16) << std::setprecision(2)
                  << distance << '\n';
    }
    return 0;
}
