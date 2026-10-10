#include <iostream>

int main()
{
    for (int stir = 1; stir <= 5; ++stir) {
        std::cout << "stir number " << stir << '\n';
    }

    const int full = 4;
    int spoons = 0;
    while (spoons < full) {
        spoons = spoons + 1;
        std::cout << "added a spoon of water, cup has " << spoons << '\n';
        if (spoons == full - 1) {
            std::cout << "  almost full: go slowly\n";
        }
    }
    std::cout << "cup is full\n";
    return 0;
}
