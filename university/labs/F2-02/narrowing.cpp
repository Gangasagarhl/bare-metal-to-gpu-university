#include <iostream>

int main()
{
    int cups{2.5};    // braces forbid a conversion that loses information
    std::cout << "cups = " << cups << '\n';
    return 0;
}
