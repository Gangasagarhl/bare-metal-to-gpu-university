#include <iostream>

int main()
{
    int cups = 2.5;   // compiles with the course flags; the .5 is lost without a word
    std::cout << "cups = " << cups << '\n';
    return 0;
}
