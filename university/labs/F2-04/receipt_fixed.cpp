#include <iomanip>
#include <iostream>

// Different names for different units: the call says which one it means.
void print_cents(int cents)
{
    std::cout << "  price: " << cents / 100 << '.' << std::setw(2) << std::setfill('0')
              << cents % 100 << '\n';
}

void print_amount(double amount)
{
    std::cout << "  price: " << std::fixed << std::setprecision(2) << amount << '\n';
}

int main()
{
    std::cout << "Soup (3 coins):\n";
    print_amount(3);
    std::cout << "Tea (2.5 coins):\n";
    print_amount(2.5);
    std::cout << "Bread (150 cents):\n";
    print_cents(150);
    return 0;
}
