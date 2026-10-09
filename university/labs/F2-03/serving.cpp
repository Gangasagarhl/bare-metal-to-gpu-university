#include <iostream>

int main()
{
    int temperature = 0;
    while (std::cin >> temperature) {
        std::cout << temperature << " C: ";
        if (temperature > 60) {
            std::cout << "warm, reheat a little\n";
        } else if (temperature > 75) {
            std::cout << "hot, serve now\n";
        } else {
            std::cout << "cold, back to the stove\n";
        }
    }
    return 0;
}
