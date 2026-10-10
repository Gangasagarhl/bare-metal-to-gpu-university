#include <iostream>

int main()
{
    int soups = 0;
    int noodles = 0;
    int unknown = 0;
    char code = ' ';

    // Read one order code after another until the input ends or the shift ends.
    while (std::cin >> code) {
        if (code == 'q') {
            std::cout << "Kitchen closes.\n";
            break;                       // leaves the while loop
        }
        switch (code) {
        case 's':
            ++soups;
            break;                       // leaves the switch only
        case 'n':
        case 'N':                        // two labels, one action
            ++noodles;
            break;
        default:
            ++unknown;
            std::cout << "Unknown order code: " << code << '\n';
            break;
        }
    }

    std::cout << "Soups: " << soups << ", noodles: " << noodles
              << ", unknown: " << unknown << '\n';

    int tables = 3;
    int guests_per_table = 0;            // nobody has sat down yet
    // && stops early: the division only happens when guests_per_table > 0.
    if (guests_per_table > 0 && (soups + noodles) / guests_per_table > 2) {
        std::cout << "Busy evening!\n";
    } else {
        std::cout << "Quiet evening at " << tables << " tables.\n";
    }
    return 0;
}
