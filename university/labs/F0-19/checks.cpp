// Number check for F0-19: recomputes every number used in the chapter text.
#include <iostream>

int main()
{
    std::cout << "10 jobs, teams of 4: " << (10 + 3) / 4 << " teams, " << ((10 + 3) / 4) * 4 << " helpers, "
              << ((10 + 3) / 4) * 4 - 10 << " idle\n";
    std::cout << "1000 jobs, teams of 256: " << (1000 + 255) / 256 << " teams, " << ((1000 + 255) / 256) * 256
              << " helpers, " << ((1000 + 255) / 256) * 256 - 1000 << " idle\n";
    int countLess = 0;
    int countLessEq = 0;
    for (int i = 0; i <= 20; ++i) {
        if (i < 10) {
            ++countLess;
        }
        if (i <= 10) {
            ++countLessEq;
        }
    }
    std::cout << "whole numbers with 0 <= i < 10: " << countLess << "; with 0 <= i <= 10: " << countLessEq << "\n";
    std::cout << "2x + 1 < 9 holds for whole x:";
    for (int x = 0; x <= 6; ++x) {
        if (2 * x + 1 < 9) {
            std::cout << " " << x;
        }
    }
    std::cout << "\n";
    std::cout << "egg tray 0..11 has " << 11 - 0 + 1 << " cups\n";
    std::cout << "x + 3 <= 7: x <= " << 7 - 3 << "\n";
    std::cout << "-2x > 6 holds for x in -6..0:";
    for (int x = -6; x <= 0; ++x) {
        if (-2 * x > 6) {
            std::cout << " " << x;
        }
    }
    std::cout << "\n";
    std::cout << "teams b with b x 256 >= 1000: smallest b = ";
    int b = 0;
    while (b * 256 < 1000) {
        ++b;
    }
    std::cout << b << "\n";
    std::cout << "3 x 256 = " << 3 * 256 << "; 4 x 256 = " << 4 * 256 << "\n";
    std::cout << "50 jobs, teams of 8: " << (50 + 7) / 8 << " teams, " << ((50 + 7) / 8) * 8 - 50 << " idle\n";
    std::cout << "players 2 <= p <= 8: 5 is " << ((2 <= 5 && 5 <= 8) ? "inside" : "outside")
              << ", 9 is " << ((2 <= 9 && 9 <= 8) ? "inside" : "outside") << "\n";
    std::cout << "tickets 10 jobs teams of 4:";
    for (int team = 0; team < 3; ++team) {
        for (int seat = 0; seat < 4; ++seat) {
            std::cout << " " << team * 4 + seat;
        }
    }
    std::cout << "\n";
    std::cout << "2x + 1 < 9: 2x < " << 9 - 1 << ", x < " << (9 - 1) / 2 << "; -2x > 6: x < " << 6 / -2 << "\n";
    std::cout << "team 3 seat 231 with 256: " << 3 * 256 + 231 << "; team 3 seat 232: " << 3 * 256 + 232 << "\n";
    std::cout << "50 / 8 = " << 50 / 8 << " r " << 50 % 8 << "; 7 x 8 = " << 7 * 8 << "\n";
    std::cout << "13 / 4 = " << 13 / 4 << "; 12 - 10 = " << 12 - 10 << "; 999 / 256 = " << 999 / 256 << " r " << 999 % 256
              << "; 6 x 8 = " << 6 * 8 << "; 56 - 50 = " << 56 - 50 << "\n";
    std::cout << "-2 x -4 = " << -2 * -4 << "; -2 x -3 = " << -2 * -3 << "; 1025 jobs, 256: teams " << (1025 + 255) / 256
              << ", helpers " << ((1025 + 255) / 256) * 256 << ", idle " << ((1025 + 255) / 256) * 256 - 1025 << "\n";
    return 0;
}
