// F9-02 Listing 1: a simulated light sensor looking down at the floor.
#include <iostream>
#include <vector>

int main()
{
    // How bright each floor square really is (simulator units, 0 = black).
    // Squares 4, 5 and 6 are covered by the dark line.
    const std::vector<int> floor = {80, 82, 79, 81, 20, 18, 22, 80, 78, 81};
    // Small wobbles a real sensor adds to every reading ("noise").
    const std::vector<int> noise = {3, -2, 1, -4, 0, 2, -1, 4, -3, 1};
    const int threshold = 50;   // darker than this means "line"

    for (int square = 0; square < 10; ++square) {
        int reading = floor[square] + noise[square];   // what the sensor reports
        std::cout << "square " << square << ": true " << floor[square]
                  << ", sensor reads " << reading << " -> ";
        if (reading < threshold) {
            std::cout << "LINE\n";
        } else {
            std::cout << "floor\n";
        }
    }
    return 0;
}
