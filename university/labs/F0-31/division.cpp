#include <iomanip>
#include <iostream>

int main()
{
    std::cout << "7 / 2   = " << 7 / 2 << '\n';
    std::cout << "7 % 2   = " << 7 % 2 << '\n';
    std::cout << "7.0 / 2 = " << 7.0 / 2 << '\n';
    std::cout << "0.1 + 0.2 = " << 0.1 + 0.2 << '\n';
    std::cout << "0.1 + 0.2 = " << std::setprecision(17) << 0.1 + 0.2
              << " (17 digits shown)\n";
    return 0;
}
