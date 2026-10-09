#include <iostream>

// Declaration: "a function with this name and these types exists somewhere".
// Its definition (the body) lives in prices.cpp.
int dish_price(int dish_number);

int main()
{
    int total = dish_price(1) + 2 * dish_price(2);
    std::cout << "Table 4: one soup and two noodles cost " << total << " coins.\n";
    return 0;
}
