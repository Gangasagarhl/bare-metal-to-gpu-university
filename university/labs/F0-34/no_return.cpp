#include <iostream>

int triple(int number)
{
    int result = number * 3;
    std::cout << "Tripled: " << result << '\n';
}

int main()
{
    std::cout << triple(4) << '\n';
    return 0;
}
