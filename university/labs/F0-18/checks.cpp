// Number check for F0-18: recomputes every number used in the chapter text.
#include <iostream>

int main()
{
    std::cout << "y = 2x + 1:";
    for (int x = 0; x <= 4; ++x) {
        std::cout << " (" << x << ", " << 2 * x + 1 << ")";
    }
    std::cout << "\n";
    std::cout << "robot d = 10 + 15t: t=0 " << 10 + 15 * 0 << ", t=4 " << 10 + 15 * 4 << ", t=6 "
              << 10 + 15 * 6 << ", t=7 " << 10 + 15 * 7 << ", t=8 " << 10 + 15 * 8 << "\n";
    std::cout << "time to reach 130 cm: (130 - 10) / 15 = " << (130 - 10) / 15 << "\n";
    std::cout << "change per second between t=2 and t=5: (" << 10 + 15 * 5 << " - " << 10 + 15 * 2
              << ") / 3 = " << ((10 + 15 * 5) - (10 + 15 * 2)) / 3 << "\n";
    std::cout << "teams of 4, jobs 1..12:";
    for (int jobs = 1; jobs <= 12; ++jobs) {
        std::cout << " " << jobs << "->" << (jobs + 4 - 1) / 4;
    }
    std::cout << "\n";
    std::cout << "f(x) = 3x - 2: f(0) = " << 3 * 0 - 2 << ", f(2) = " << 3 * 2 - 2 << ", f(5) = " << 3 * 5 - 2 << "\n";
    std::cout << "slow robot d = 5 + 10t at t=3: " << 5 + 10 * 3 << "\n";
    std::cout << "start added twice at t=6: 10 + 10 + 15 x 6 = " << 10 + 10 + 15 * 6 << "\n";
    std::cout << "teams of 256 for 1000 jobs: " << (1000 + 255) / 256 << "; for 1025 jobs: " << (1025 + 255) / 256 << "\n";
    std::cout << "y = 20 - 4x: x=0 " << 20 << ", x=5 " << 20 - 4 * 5 << "\n";
    std::cout << "130 - 10 = " << 130 - 10 << "; 120 / 15 = " << 120 / 15 << "; 15 x 8 = " << 15 * 8 << "\n";
    std::cout << "y = 20 - 4x: x=1 " << 20 - 4 * 1 << ", x=2 " << 20 - 4 * 2 << "; slope (12 - 16) / (2 - 1) = " << (12 - 16) / (2 - 1) << "\n";
    std::cout << "85 - 40 = " << 85 - 40 << "; 5 - 2 = " << 5 - 2 << "; 45 / 3 = " << 45 / 3 << "\n";
    std::cout << "10 jobs, teams of 4: " << (10 + 3) / 4 << "; 15 x 6 = " << 15 * 6 << "; 110 - 100 = " << 110 - 100 << "\n";
    std::cout << "10 + 15 x 6.5 = " << 10 + 15 * 6.5 << "; 10 + 15t = 100: t = " << (100 - 10) / 15 << "; 70 / 4 = " << 70 / 4.0
              << "; 9 / 4 = " << 9 / 4.0 << "\n";
    std::cout << "buggy rule at t = 0: 10 + 10 + 15 x 0 = " << 10 + 10 + 15 * 0 << "\n";
    return 0;
}
