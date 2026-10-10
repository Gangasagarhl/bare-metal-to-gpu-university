#include <iostream>
#include <string>

int main()
{
    int cups = 7;
    double litres = 1.5;
    bool is_hot = true;
    char letter = 'A';
    std::string dish = "soup";

    std::cout << "cups: " << cups << '\n';
    std::cout << "litres: " << litres << '\n';
    std::cout << "is_hot: " << is_hot << '\n';
    std::cout << "is_hot with boolalpha: " << std::boolalpha << is_hot << '\n';
    std::cout << "letter: " << letter << '\n';
    std::cout << "letter as a number: " << static_cast<int>(letter) << '\n';
    std::cout << "dish: " << dish << ", letters in it: " << dish.size() << '\n';
    return 0;
}
