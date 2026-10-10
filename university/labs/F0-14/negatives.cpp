// Listing 1 (F0-14): moving along the number line in a lift.
#include <iostream>

int main()
{
    int floor = 3;
    std::cout << "start on floor " << floor << "\n";

    floor = floor - 7;
    std::cout << "go down 7 floors: " << floor << "\n";

    floor = floor + 2;
    std::cout << "go up 2 floors:   " << floor << "\n";

    int morning = -3;
    int afternoon = 5;
    std::cout << "temperature change: " << afternoon - morning << "\n";
    return 0;
}
