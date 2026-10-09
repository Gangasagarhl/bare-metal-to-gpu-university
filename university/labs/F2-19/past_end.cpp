#include <iostream>

// Check-yourself question 5: computing one past the end is fine; reading it is not.
int main()
{
    std::cout << std::unitbuf;
    int shelf[4] = {10, 20, 30, 40};
    int* q = &shelf[0] + 4;                          // one past the end: allowed
    std::cout << "q - &shelf[0] = " << (q - &shelf[0]) << '\n';
    std::cout << "*q = " << *q << '\n';              // reading it: undefined behaviour
    return 0;
}
