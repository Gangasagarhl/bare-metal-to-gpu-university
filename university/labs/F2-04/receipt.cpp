#include <iomanip>
#include <iostream>

// Price given in cents (a whole number).
void print_price(int cents)
{
    std::cout << "  price: " << cents / 100 << '.' << std::setw(2) << std::setfill('0')
              << cents % 100 << '\n';
}

// Price given as an amount with a fraction.
void print_price(double amount)
{
    std::cout << "  price: " << std::fixed << std::setprecision(2) << amount << '\n';
}

int main()
{
    std::cout << "Soup (3 coins):\n";
    print_price(3);
    std::cout << "Tea (2.5 coins):\n";
    print_price(2.5);
    std::cout << "Bread (150 cents):\n";
    print_price(150);
    return 0;
}
