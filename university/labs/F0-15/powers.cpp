// Listing 1 (F0-15): doubling, again and again.
#include <iostream>

int main()
{
    long long value = 1;
    for (int n = 0; n <= 16; ++n) {
        std::cout << "2^" << n << " = " << value << "\n";
        value = value * 2;
    }
    return 0;
}
