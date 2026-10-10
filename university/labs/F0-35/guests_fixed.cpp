#include <iostream>
#include <string>
#include <vector>

int main()
{
    std::vector<std::string> guests = {"Lena", "Kofi", "Mei", "Ravi"};
    std::cout << "Guests invited: " << guests.size() << '\n';
    std::cout << "Name cards printed:\n";
    for (const std::string& guest : guests) {
        std::cout << "  [" << guest << "]\n";
    }
    return 0;
}
