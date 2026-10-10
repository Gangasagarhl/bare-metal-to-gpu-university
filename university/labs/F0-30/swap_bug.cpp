#include <iostream>

int main()
{
    int red_cup = 7;
    int blue_cup = 2;
    std::cout << "Before: red cup = " << red_cup << ", blue cup = " << blue_cup << '\n';

    red_cup = blue_cup;
    blue_cup = red_cup;
    std::cout << "After:  red cup = " << red_cup << ", blue cup = " << blue_cup << '\n';
    return 0;
}
