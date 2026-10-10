// F0-56 checks: recompute every number used in the chapter text.
#include <cmath>
#include <iostream>

int main()
{
    // Worked example: drawing socks from a drawer of 3 red + 2 blue, two draws without putting back.
    const double pRedRed = (3.0 / 5.0) * (2.0 / 4.0);
    const double pBlueBlue = (2.0 / 5.0) * (1.0 / 4.0);
    std::cout << "P(red, red) = 3/5 * 2/4 = " << pRedRed << "\n";
    std::cout << "P(blue, blue) = 2/5 * 1/4 = " << pBlueBlue << "\n";
    std::cout << "P(a matching pair) = " << pRedRed + pBlueBlue << "\n";
    std::cout << "P(not matching) = " << 1.0 - (pRedRed + pBlueBlue) << "\n";
    // Counting check of the same: C(5,2) = 10 unordered pairs; C(3,2) + C(2,2) = 3 + 1 matching.
    std::cout << "matching pairs by counting = 4/10 = " << 4.0 / 10.0 << "\n";
    // Two coins: P(at least one head) = 1 - (1/2)^2.
    std::cout << "P(at least one head in 2 tosses) = " << 1.0 - std::pow(0.5, 2) << "\n";
    // At least one six in k rolls, exact values.
    for (int k : {1, 4, 6, 7}) {
        std::cout << "P(at least one six in " << k << " rolls) = 1 - (5/6)^" << k << " = "
                  << 1.0 - std::pow(5.0 / 6.0, k) << "\n";
    }
    // Two dice: P(double) = 6/36, P(sum 7 or double) = 12/36 (disjoint: a double has an even sum).
    std::cout << "P(double) = " << 6.0 / 36.0 << ", P(sum 7 or double) = " << 12.0 / 36.0 << "\n";
    // Overlapping events: P(red die is 6 or blue die is 6) = 1/6 + 1/6 - 1/36.
    std::cout << "P(red 6 or blue 6) = 11/36 = " << 11.0 / 36.0 << "\n";
    // Check-yourself answers.
    std::cout << "three dice: " << 6 * 6 * 6 << " outcomes, P(6,6,6) = " << 1.0 / 216.0 << "\n";
    std::cout << "P(even total) = 18/36 = " << 18.0 / 36.0 << ", P(total 8) = " << 5.0 / 36.0 << "\n";
    std::cout << "7! = " << 7 * 6 * 5 * 4 * 3 * 2 << ", C(7,2) = " << 7 * 6 / 2 << "\n";
    std::cout << "P(at least one of 4 encoders bad) = 1 - 0.95^4 = " << 1.0 - std::pow(0.95, 4) << "\n";
    std::cout << "with replacement: P(match) = " << 0.6 * 0.6 + 0.4 * 0.4 << "\n";
    return 0;
}
