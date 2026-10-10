// Listing 1 (F0-18): the university's tiny toy-robot simulator prints a distance table.
#include <iomanip>
#include <iostream>

int distanceAt(int seconds)
{
    int start = 10;  // centimetres from the door when the timer starts
    int speed = 15;  // centimetres travelled each second in this simulator
    return start + speed * seconds;
}

int main()
{
    std::cout << "time (s) | distance (cm) | bar (one # per 10 cm)\n";
    for (int t = 0; t <= 8; ++t) {
        int d = distanceAt(t);
        std::cout << std::setw(8) << t << " | " << std::setw(13) << d << " | ";
        for (int k = 0; k < d / 10; ++k) {
            std::cout << '#';
        }
        std::cout << "\n";
    }
    return 0;
}
