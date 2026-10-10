// Evidence program for the forensic lab: Tomas's first colour mixer.
#include <iostream>

int main()
{
    int red = 0;
    int green = 0;
    int blue = 0;
    while (std::cin >> red >> green >> blue) {
        std::cout << red << ' ' << green << ' ' << blue << " -> #"
                  << std::hex << std::uppercase << red << green << blue << std::dec << '\n';
    }
    return 0;
}
