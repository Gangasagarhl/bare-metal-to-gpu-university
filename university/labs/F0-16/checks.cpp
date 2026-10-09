// Number check for F0-16: recomputes every number used in the chapter text.
#include <iostream>

int main()
{
    for (int n = 1; n <= 4; ++n) {
        std::cout << "n = " << n << ": eggs 2n = " << 2 * n << ", flour 150n = " << 150 * n << "\n";
    }
    int n = 5;
    std::cout << "3n + 2 with n = 5: " << 3 * n + 2 << "\n";
    n = 4;
    std::cout << "2(n + 3) with n = 4: " << 2 * (n + 3) << "; 2n + 3 with n = 4: " << 2 * n + 3 << "\n";
    int a = 2;
    std::cout << "4a + 3a with a = 2: " << 4 * a + 3 * a << " = 7a = " << 7 * a << "\n";
    std::cout << "tickets with 4 seats per table:\n";
    for (int t = 0; t < 3; ++t) {
        std::cout << "  table " << t << ":";
        for (int s = 0; s < 4; ++s) {
            std::cout << " " << t * 4 + s;
        }
        std::cout << "\n";
    }
    std::cout << "table 2 seat 3: " << 2 * 4 + 3 << "; table 0 seat 0: " << 0 * 4 + 0
              << "; table 5 seat 1: " << 5 * 4 + 1 << "\n";
    std::cout << "256 seats, table 3 seat 10: " << 3 * 256 + 10 << "\n";
    std::cout << "price 3p + 5 with p = 4: " << 3 * 4 + 5 << "\n";
    std::cout << "x + x + x with x = 6: " << 6 + 6 + 6 << " = 3x = " << 3 * 6 << "\n";
    std::cout << "5b - 2b + 1 with b = 3: " << 5 * 3 - 2 * 3 + 1 << " = 3b + 1 = " << 3 * 3 + 1 << "\n";
    std::cout << "perimeter 2l + 2w with l = 7, w = 3: " << 2 * 7 + 2 * 3 << "\n";
    std::cout << "ticket 14 with 4 seats: table " << 14 / 4 << ", seat " << 14 % 4 << "\n";
    n = -2;
    std::cout << "3n with n = -2: " << 3 * n << "; n + 3 + 2 with n = 4: " << 4 + 3 + 2 << "; 2 x 4 x 3 = " << 2 * 4 * 3 << "\n";
    std::cout << "2n + 6 with n = 4: " << 2 * 4 + 6 << "; 3 x 4 = " << 3 * 4 << " tickets for 3 tables of 4\n";
    std::cout << "3a + 2b with a = 1, b = 10: " << 3 * 1 + 2 * 10 << "; 5ab: " << 5 * 1 * 10 << "; 3n + 2 with n = 1: " << 3 * 1 + 2
              << "; 15 - 6 + 1 = " << 15 - 6 + 1 << "\n";
    return 0;
}
