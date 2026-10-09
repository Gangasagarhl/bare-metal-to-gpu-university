// Listing 2 (F0-15): how many bits are in a byte on this machine, and how many patterns they make.
#include <climits>
#include <iostream>

int main()
{
    int bits = CHAR_BIT;
    long long patterns = 1;
    for (int i = 0; i < bits; ++i) {
        patterns = patterns * 2;
    }
    std::cout << "bits in one byte here: " << bits << "\n";
    std::cout << "patterns of " << bits << " switches: " << patterns << "\n";
    return 0;
}
