#include <iostream>

int main()
{
    int cookies;
    std::cout << "How many cookies are in the jar? ";
    std::cin >> cookies;
    std::cout << '\n';

    const int baked = 12;
    cookies = cookies + baked;
    std::cout << "We baked " << baked << " more. Now there are " << cookies << ".\n";
    return 0;
}
