#include <climits>
#include <iostream>

// Signed overflow is undefined behaviour; UndefinedBehaviorSanitizer reports it.
int addPortions(int a, int b)
{
    return a + b;
}

int main()
{
    std::cout << std::unitbuf;
    const int big = INT_MAX;
    std::cout << "INT_MAX = " << big << '\n';
    std::cout << "INT_MAX + 1 = " << addPortions(big, 1) << '\n';
    unsigned int u = UINT_MAX;
    u = u + 1;                       // unsigned arithmetic wraps: defined, result 0
    std::cout << "UINT_MAX + 1 = " << u << '\n';
    return 0;
}
