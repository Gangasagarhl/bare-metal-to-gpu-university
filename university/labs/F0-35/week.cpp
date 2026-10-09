#include <array>
#include <iostream>
#include <string>

int main()
{
    std::array<std::string, 3> meals = {"breakfast", "lunch", "dinner"};
    std::cout << "Meals in a day: " << meals.size() << '\n';
    for (const std::string& meal : meals) {
        std::cout << "- " << meal << '\n';
    }
    std::cout << "Last meal: " << meals.at(2) << '\n';
    return 0;
}
