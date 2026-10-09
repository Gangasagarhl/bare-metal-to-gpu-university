#include <iostream>

int main()
{
    int soups = 0;
    char code = ' ';
    while (std::cin >> code) {
        switch (code) {
        case 'q':
            std::cout << "Kitchen closes.\n";
            break;                       // leaves only the switch, not the loop
        case 's':
            ++soups;
            break;
        default:
            std::cout << "Unknown order code: " << code << '\n';
            break;
        }
    }
    std::cout << "Soups: " << soups << '\n';
    return 0;
}
