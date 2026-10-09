// Lab program (F0-17): Listing 1 with a, b and c read from the input, to try your own equations.
#include <iostream>

int main()
{
    int a = 0;
    int b = 0;
    int c = 0;
    std::cin >> a >> b >> c;
    if (a == 0) {
        std::cout << "a must not be 0: you cannot share into 0 groups\n";
        return 1;
    }

    int afterTakingAway = c - b;
    int x = afterTakingAway / a;
    std::cout << "solve " << a << "x + " << b << " = " << c << "\n";
    std::cout << "take " << b << " from both sides: " << a << "x = " << afterTakingAway << "\n";
    std::cout << "share both sides into " << a << ": x = " << x << " (whole-number division)\n";

    int left = a * x + b;
    std::cout << "check: " << a << " x " << x << " + " << b << " = " << left << "\n";
    if (left == c) {
        std::cout << "balanced\n";
    } else {
        std::cout << "not balanced: there is no whole-number answer\n";
    }
    return 0;
}
