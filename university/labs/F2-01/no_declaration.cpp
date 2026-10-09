#include <iostream>

int main()
{
    int total = dish_price(1) + 2 * dish_price(2);
    std::cout << "Table 4 pays " << total << " coins.\n";
    return 0;
}

int dish_price(int dish_number)
{
    if (dish_number == 1) {
        return 7;
    }
    return 12;
}
