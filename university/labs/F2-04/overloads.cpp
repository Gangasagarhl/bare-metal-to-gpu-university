#include <iostream>

// Declarations first: the definitions are below main.
int portions(int guests);
int portions(int guests, int per_guest);
double portions(double kilograms_of_rice, double kilograms_per_portion = 0.1);

int main()
{
    std::cout << "portions(4)       = " << portions(4) << '\n';
    std::cout << "portions(4, 3)    = " << portions(4, 3) << '\n';
    std::cout << "portions(2.0)     = " << portions(2.0) << '\n';
    std::cout << "portions(2.0, 0.25) = " << portions(2.0, 0.25) << '\n';
    return 0;
}

int portions(int guests)
{
    return guests;                       // one portion each
}

int portions(int guests, int per_guest)
{
    return guests * per_guest;
}

double portions(double kilograms_of_rice, double kilograms_per_portion)
{
    return kilograms_of_rice / kilograms_per_portion;
}
