// Hex colour mixer: read red, green and blue (each 0 to 255) and print #RRGGBB.
#include <iomanip>
#include <iostream>

void printTwoHexDigits(int amount)
{
    std::cout << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
              << amount << std::dec;
}

int main()
{
    int red = 0;
    int green = 0;
    int blue = 0;
    while (std::cin >> red >> green >> blue) {
        if (red < 0 || red > 255 || green < 0 || green > 255 || blue < 0 || blue > 255) {
            std::cout << red << ' ' << green << ' ' << blue
                      << " -> each amount must be from 0 to 255\n";
            continue;
        }
        std::cout << red << ' ' << green << ' ' << blue << " -> #";
        printTwoHexDigits(red);
        printTwoHexDigits(green);
        printTwoHexDigits(blue);
        std::cout << '\n';
    }
    return 0;
}
