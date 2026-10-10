#include <iostream>
#include <string>
#include <vector>

std::string switchesFor(int number)
{
    const std::vector<int> placeValues = {8, 4, 2, 1};
    std::string row;
    for (const int value : placeValues) {
        if (number >= value) {
            row += " ON ";
            number = number - value;
        } else {
            row += " off";
        }
    }
    return row;
}

int main()
{
    std::cout << "number | 8s  4s  2s  1s\n";
    for (int number = 0; number <= 15; ++number) {
        std::cout << "  " << number << (number < 10 ? " " : "") << "   |" << switchesFor(number) << '\n';
    }
    return 0;
}
