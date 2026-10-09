// Forensic evidence generator: what a voltmeter shows between two points.
// Each point has a potential measured from the reference point "ground" (0 V).
// The meter shows potential(red probe) - potential(black probe).
// All potentials are exercise numbers for one imagined circuit, not measurements.
#include <iomanip>
#include <iostream>
#include <map>
#include <string>

int main()
{
    const std::map<std::string, double> potential = {
        {"GND", 0.0}, {"A", 6.0}, {"B", 4.5}, {"C", 1.5}};

    std::string who;
    std::string red;
    std::string black;
    std::cout << std::fixed << std::setprecision(2);
    while (std::cin >> who >> red >> black) {
        const double reading = potential.at(red) - potential.at(black);
        std::cout << who << " (red on " << red << ", black on " << black << "): "
                  << std::setw(6) << reading << " V\n";
    }
    return 0;
}
