#include <iostream>
#include <string>
#include <vector>

int main()
{
    const std::vector<int> placeValues = {1, 2, 4, 8, 16, 32, 64, 128};
    std::string group;
    std::string message;
    while (std::cin >> group) {
        int number = 0;
        for (std::size_t i = 0; i < group.size() && i < placeValues.size(); ++i) {
            if (group[i] == '1') {
                number = number + placeValues[i];
            }
        }
        const bool printable = number >= 32 && number <= 126;
        const char letter = printable ? static_cast<char>(number) : '?';
        std::cout << group << " -> " << number << " -> " << letter << '\n';
        message += letter;
    }
    std::cout << "Message: " << message << '\n';
    return 0;
}
