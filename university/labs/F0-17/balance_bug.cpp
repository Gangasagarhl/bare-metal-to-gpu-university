// Forensic evidence for F0-17: an equation solved with one wrong step (deliberate).
#include <iostream>

int main()
{
    double c = 14.0;
    double step1 = c;          // 2 was taken from the left side only
    double x = step1 / 3.0;
    double check = 3.0 * x + 2.0;

    std::cout << "Solve 3x + 2 = 14\n";
    std::cout << "step 1: take 2 away: 3x = " << step1 << "\n";
    std::cout << "step 2: divide by 3: x = " << x << "\n";
    std::cout << "check:  3 x " << x << " + 2 = " << check << " (should be 14)\n";
    return 0;
}
