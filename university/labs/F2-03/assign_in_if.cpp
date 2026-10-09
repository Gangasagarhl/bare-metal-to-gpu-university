#include <iostream>

int main()
{
    int guests = 4;
    if (guests = 0) {                    // meant: guests == 0
        std::cout << "Nobody is coming.\n";
    }
    std::cout << "Guests: " << guests << '\n';
    return 0;
}
