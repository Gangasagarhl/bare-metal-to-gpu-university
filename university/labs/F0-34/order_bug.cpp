#include <iostream>

int main()
{
    std::cout << "Make enough for " << double_it(3) << " plates.\n";
    return 0;
}

int double_it(int number)
{
    return number * 2;
}
