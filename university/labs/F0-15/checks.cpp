// Number check for F0-15: recomputes every number used in the chapter text.
#include <iostream>

long long power(long long base, int exponent)
{
    long long result = 1;
    for (int i = 0; i < exponent; ++i) {
        result = result * base;
    }
    return result;
}

int main()
{
    for (int n = 0; n <= 6; ++n) {
        std::cout << "10^" << n << " = " << power(10, n) << "\n";
    }
    std::cout << "2^3 = " << power(2, 3) << "; 2^4 = " << power(2, 4) << "; 2^5 = " << power(2, 5)
              << "; 2 x 5 = " << 2 * 5 << "\n";
    std::cout << "2^8 = " << power(2, 8) << "; 2^10 = " << power(2, 10) << "; 2^10 - 10^3 = "
              << power(2, 10) - power(10, 3) << "\n";
    std::cout << "2^16 = " << power(2, 16) << "; 2^20 = " << power(2, 20) << "; 2^30 = " << power(2, 30) << "\n";
    std::cout << "10^6 = " << power(10, 6) << "; 2^20 - 10^6 = " << power(2, 20) - power(10, 6) << "\n";
    std::cout << "5 folds: " << power(2, 5) << " layers; 7 folds: " << power(2, 7) << " layers\n";
    int halvings = 0;
    for (long long x = 1024; x > 1; x = x / 2) {
        ++halvings;
    }
    std::cout << "halvings from 1024 to 1: " << halvings << "\n";
    std::cout << "2^3 x 2^2 = " << power(2, 3) * power(2, 2) << " = 2^5 = " << power(2, 5) << "\n";
    std::cout << "10^2 x 10^3 = " << power(10, 2) * power(10, 3) << " = 10^5\n";
    std::cout << "4,000,000 = 4 x 10^6 = " << 4 * power(10, 6) << "\n";
    std::cout << "1000 jobs, teams of 256 = 2^8: " << (1000 + 256 - 1) / 256 << " teams, "
              << ((1000 + 256 - 1) / 256) * 256 << " helpers\n";
    std::cout << "1024 jobs, teams of 256: " << (1024 + 256 - 1) / 256 << " teams\n";
    std::cout << "2^7 = " << power(2, 7) << "; 2^9 = " << power(2, 9) << "; 2^12 = " << power(2, 12) << "\n";
    std::cout << "3 switches: " << power(2, 3) << " patterns; 4 switches: " << power(2, 4) << " patterns\n";
    std::cout << "2^0 = " << power(2, 0) << "; 10^0 = " << power(10, 0) << "\n";
    std::cout << "256 / 2 / 2 / 2 = " << 256 / 2 / 2 / 2 << "\n";
    std::cout << "halving 1024:";
    for (long long x = 1024; x >= 1; x = x / 2) {
        std::cout << " " << x;
    }
    std::cout << "\n";
    std::cout << "24 / 1000 as percent = " << 24.0 / 1000 * 100 << " %; 1000 helpers of 1024 idle = " << 1024 - 1000 << "\n";
    std::cout << "10 x 10 x 10 = " << 10 * 10 * 10 << "; 2 x 2 x 2 x 2 x 2 = " << 2 * 2 * 2 * 2 * 2 << "\n";
    std::cout << "6 switches: " << power(2, 6) << " patterns; 2^4 x 2^3 = " << power(2, 4) * power(2, 3) << " = 2^7\n";
    int h = 0;
    for (long long x = 256; x > 1; x = x / 2) {
        ++h;
    }
    std::cout << "halvings from 256 to 1: " << h << "; 100 x 1000 = " << 100 * 1000 << "; 16 x 8 = " << 16 * 8 << "\n";
    std::cout << "1024 jobs, teams of 256: helpers " << 4 * 256 << ", idle " << 4 * 256 - 1024 << "\n";
    return 0;
}
