// Check two laws with three inputs by trying all 8 combinations.
#include <iostream>

int main()
{
    int failures = 0;
    for (bool a : {false, true}) {
        for (bool b : {false, true}) {
            for (bool c : {false, true}) {
                const bool distLeft = a && (b || c);
                const bool distRight = (a && b) || (a && c);
                const bool dmLeft = !(a || b || c);
                const bool dmRight = !a && !b && !c;
                const bool distributive = (distLeft == distRight);
                const bool deMorgan3 = (dmLeft == dmRight);
                std::cout << a << b << c << "  distributive " << distributive
                          << "  De Morgan for three " << deMorgan3 << '\n';
                if (!distributive || !deMorgan3) {
                    ++failures;
                }
            }
        }
    }
    std::cout << "combinations where a law failed: " << failures << '\n';
    return 0;
}
