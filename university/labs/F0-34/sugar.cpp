#include <iostream>

void add_sugar(int spoons)
{
    spoons = spoons + 1;
    std::cout << "Inside add_sugar: spoons = " << spoons << '\n';
}

int main()
{
    int spoons = 2;
    std::cout << "Before: spoons = " << spoons << '\n';
    add_sugar(spoons);
    std::cout << "After:  spoons = " << spoons << '\n';
    return 0;
}
