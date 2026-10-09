#include <iostream>

int portions(int guests, int per_guest)
{
    return guests * per_guest;
}

double portions(double kilograms_of_rice, double kilograms_per_portion)
{
    return kilograms_of_rice / kilograms_per_portion;
}

int main()
{
    std::cout << portions(4.0, 3) << '\n';   // one double, one int
    return 0;
}
