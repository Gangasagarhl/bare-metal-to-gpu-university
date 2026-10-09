#include <iostream>

int main()
{
    const int darkLimit = 30;
    int light = 0;
    while (std::cin >> light) {
        std::cout << "input: light level " << light << "  ->  output: ";
        if (light < darkLimit) {
            std::cout << "lamp ON\n";
        } else {
            std::cout << "lamp off\n";
        }
    }
    return 0;
}
