#include <iostream>

int main()
{
    int apples = 3;
    std::cout << "The apple jar holds " << apples << '\n';

    apples = apples + 2;
    std::cout << "After adding 2, it holds " << apples << '\n';

    int pears = apples;
    apples = 0;
    std::cout << "Apples: " << apples << ", pears: " << pears << '\n';
    return 0;
}
