#include <iostream>

int main()
{
    char size = 'S';
    int grams = 0;
    switch (size) {
    case 'S':
        grams = 200;                     // the break is missing here
    case 'L':
        grams = 400;
        break;
    default:
        grams = 0;
        break;
    }
    std::cout << "Portion: " << grams << " g\n";
    return 0;
}
