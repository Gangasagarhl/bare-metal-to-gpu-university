#include <iostream>

int main()
{
    int temperature = 0;
    std::cin >> temperature;
    std::cout << "It is " << temperature << " degrees outside.\n";

    if temperature < 5 {
        std::cout << "Wear a warm coat, a hat and gloves.\n";
    } else if (temperature < 18) {
        std::cout << "Wear a jumper.\n";
    } else {
        std::cout << "A T-shirt is fine.\n";
    }
    std::cout << "Have a good day!\n";
    return 0;
}
