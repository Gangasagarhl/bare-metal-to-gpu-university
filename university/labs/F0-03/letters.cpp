#include <iostream>
#include <string>
#include <vector>

std::string eightSwitches(int number)
{
    const std::vector<int> placeValues = {128, 64, 32, 16, 8, 4, 2, 1};
    std::string bits;
    for (const int value : placeValues) {
        if (number >= value) {
            bits += '1';
            number = number - value;
        } else {
            bits += '0';
        }
    }
    return bits;
}

int main()
{
    const std::string word = "Hi A!";
    for (const char letter : word) {
        const int number = static_cast<int>(letter);
        std::cout << "'" << letter << "' is stored as number " << number
                  << " = bits " << eightSwitches(number) << '\n';
    }
    return 0;
}
