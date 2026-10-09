// Two switches and one LED: in series (one after the other) or in parallel (side by side).
#include <iostream>

int main()
{
    std::cout << "A B | series LED | parallel LED\n";
    for (int a = 0; a <= 1; ++a) {
        for (int b = 0; b <= 1; ++b) {
            const bool series = (a == 1) && (b == 1);   // both must be closed
            const bool parallel = (a == 1) || (b == 1); // either one is enough
            std::cout << a << " " << b << " | "
                      << (series ? "ON " : "off") << "        | "
                      << (parallel ? "ON" : "off") << "\n";
        }
    }
    return 0;
}
