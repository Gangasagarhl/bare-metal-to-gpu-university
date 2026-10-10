// This file is EXPECTED not to compile with the university's flags (-Werror).
// It tests bit 3 without brackets around the AND.
#include <iostream>

int main()
{
    const unsigned x = 0b0000'1000;
    const unsigned k = 3;
    if (x & 1u << k != 0) {
        std::cout << "bit 3 is on\n";
    }
    return 0;
}
