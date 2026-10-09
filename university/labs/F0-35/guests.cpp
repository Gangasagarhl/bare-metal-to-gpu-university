#include <iostream>
#include <string>
#include <vector>

int main()
{
    std::vector<std::string> guests = {"Lena", "Kofi", "Mei", "Ravi"};
    std::cout << "Guests invited: " << guests.size() << '\n';
    std::cout << "Name cards printed:\n";
    for (std::size_t i = 0; i + 1 < guests.size(); ++i) {
        std::cout << "  [" << guests[i] << "]\n";
    }
    return 0;
}
