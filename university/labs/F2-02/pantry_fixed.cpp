#include <iostream>

int main()
{
    long long eggs_in_stock = 0;
    long long eggs_used = 0;
    std::cin >> eggs_in_stock >> eggs_used;

    long long eggs_left = eggs_in_stock - eggs_used;
    std::cout << "Eggs left: " << eggs_left << '\n';
    if (eggs_left < 0) {
        std::cout << "More eggs were used than were in stock: check the counts!\n";
    } else if (eggs_left < 6) {
        std::cout << "Order more eggs!\n";
    } else {
        std::cout << "Enough eggs for tomorrow.\n";
    }
    return 0;
}
