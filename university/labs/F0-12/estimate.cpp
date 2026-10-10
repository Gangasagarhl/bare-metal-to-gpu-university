// Lab program (F0-12): turn two counts from the rice-jar game into an estimate.
#include <iostream>

int main()
{
    int grainsPerSpoon = 0;
    int spoonsInJar = 0;
    std::cin >> grainsPerSpoon >> spoonsInJar;

    int roundedGrains = (grainsPerSpoon + 5) / 10 * 10;
    int roundedSpoons = (spoonsInJar + 5) / 10 * 10;

    std::cout << "grains in one spoon (counted): " << grainsPerSpoon << "\n";
    std::cout << "spoons to fill the jar:        " << spoonsInJar << "\n";
    std::cout << "quick estimate: " << roundedGrains << " x " << roundedSpoons << " = "
              << roundedGrains * roundedSpoons << "\n";
    std::cout << "full product:   " << grainsPerSpoon << " x " << spoonsInJar << " = "
              << grainsPerSpoon * spoonsInJar << "\n";
    return 0;
}
