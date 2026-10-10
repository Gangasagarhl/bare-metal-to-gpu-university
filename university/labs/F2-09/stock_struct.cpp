#include <iostream>
#include <string>

// A plain struct: every member is public, so any code can change any member.
struct ShelfStock
{
    std::string name;
    int portions;
    int capacity;
};

// The rule we want to be true at all times: 0 <= portions <= capacity.
bool rule_holds(const ShelfStock& s)
{
    return s.portions >= 0 && s.portions <= s.capacity;
}

int main()
{
    ShelfStock soup{"soup", 10, 20};
    std::cout << soup.name << ": " << soup.portions << " of " << soup.capacity
              << ", rule holds: " << rule_holds(soup) << '\n';

    soup.portions = soup.portions - 13;  // serve 13 portions without checking
    std::cout << soup.name << ": " << soup.portions << " of " << soup.capacity
              << ", rule holds: " << rule_holds(soup) << '\n';
    return 0;
}
