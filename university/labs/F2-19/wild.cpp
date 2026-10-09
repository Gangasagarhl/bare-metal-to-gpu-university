#include <iostream>

// Does not compile with the course flags: the pointer is used before it gets a value.
int main()
{
    int* p;
    *p = 5;
    std::cout << *p << '\n';
    return 0;
}
