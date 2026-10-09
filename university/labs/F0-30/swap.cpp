#include <iostream>

int main()
{
    int left = 4;
    int right = 9;
    std::cout << "Before: left = " << left << ", right = " << right << '\n';

    int spare = left;
    left = right;
    right = spare;
    std::cout << "After:  left = " << left << ", right = " << right << '\n';
    return 0;
}
