// Listing 1 (F0-17): solve a x + b = c like a balance scale, then check.
#include <iostream>

int main()
{
    int a = 3;   // bags on the left pan
    int b = 2;   // loose marbles on the left pan
    int c = 14;  // marbles on the right pan

    int afterTakingAway = c - b;
    int x = afterTakingAway / a;
    std::cout << "take " << b << " from both sides: " << a << "x = " << afterTakingAway << "\n";
    std::cout << "share both sides into " << a << ": x = " << x << "\n";

    int left = a * x + b;
    std::cout << "check: " << a << " x " << x << " + " << b << " = " << left << "\n";
    if (left == c) {
        std::cout << "balanced\n";
    } else {
        std::cout << "not balanced\n";
    }
    return 0;
}
