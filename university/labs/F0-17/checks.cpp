// Number check for F0-17: recomputes every number used in the chapter text.
#include <iostream>

int main()
{
    std::cout << "x + 5 = 12: x = " << 12 - 5 << "; check " << 7 + 5 << "\n";
    std::cout << "3x + 2 = 14: 3x = " << 14 - 2 << ", x = " << (14 - 2) / 3 << "; check " << 3 * 4 + 2 << "\n";
    std::cout << "2x - 3 = 9: 2x = " << 9 + 3 << ", x = " << (9 + 3) / 2 << "; check " << 2 * 6 - 3 << "\n";
    std::cout << "x / 4 = 5: x = " << 5 * 4 << "; check " << 20 / 4 << "\n";
    std::cout << "8t = 2000: t = " << 2000 / 8 << "; check " << 8 * 250 << "\n";
    std::cout << "4n + 6 = 30: 4n = " << 30 - 6 << ", n = " << (30 - 6) / 4 << "; check " << 4 * 6 + 6 << "\n";
    std::cout << "x - 7 = 3: x = " << 3 + 7 << "\n";
    std::cout << "5x = 35: x = " << 35 / 5 << "\n";
    std::cout << "2x + 1 = 11: x = " << (11 - 1) / 2 << "\n";
    std::cout << "6 + 3x = 21: x = " << (21 - 6) / 3 << "\n";
    std::cout << "2x + 3 = 12 needs x = " << (12 - 3) / 2.0 << "\n";
    std::cout << "wrong step: 14 / 3 = " << 14.0 / 3 << "\n";
    std::cout << "ticket 11 = table x 4 + 3: table = " << (11 - 3) / 4 << "\n";
    std::cout << "x + 4 = -2: x = " << -2 - 4 << "\n";
    std::cout << "3(x + 1) = 18: x + 1 = " << 18 / 3 << ", x = " << 18 / 3 - 1 << "\n";
    std::cout << "250t = 6000: t = " << 6000 / 250 << "; check " << 250 * 24 << "\n";
    std::cout << "x + 4 = 12: x = " << 12 - 4 << "; check 3 x 5 + 3 ... 3(5 + 1) = " << 3 * (5 + 1) << "\n";
    std::cout << "2 x 4.5 + 3 = " << 2 * 4.5 + 3 << "; 14 - 2 = " << 14 - 2 << "\n";
    std::cout << "256t = 1000: t = " << 1000.0 / 256 << "; 13 / 3 = " << 13 / 3 << " r " << 13 % 3
              << "; 3 x 4 + 4 = " << 3 * 4 + 4 << "\n";
    std::cout << "3x + 2 = x + 10: 2x = " << 10 - 2 << ", x = " << (10 - 2) / 2 << "; check " << 3 * 4 + 2 << " = " << 4 + 10 << "\n";
    std::cout << "3 x 4.67 + 2 = " << 3 * 4.67 + 2 << "\n";
    return 0;
}
