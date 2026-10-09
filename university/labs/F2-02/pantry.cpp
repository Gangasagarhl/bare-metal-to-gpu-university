#include <cstddef>
#include <iostream>

int main()
{
    std::size_t eggs_in_stock = 0;
    std::size_t eggs_used = 0;
    std::cin >> eggs_in_stock >> eggs_used;

    std::size_t eggs_left = eggs_in_stock - eggs_used;
    std::cout << "Eggs in stock: " << eggs_in_stock << '\n';
    std::cout << "Eggs used:     " << eggs_used << '\n';
    std::cout << "Eggs left:     " << eggs_left << '\n';
    if (eggs_left < 6) {
        std::cout << "Order more eggs!\n";
    } else {
        std::cout << "Enough eggs for tomorrow.\n";
    }
    return 0;
}
