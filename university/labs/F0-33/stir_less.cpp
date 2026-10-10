#include <iostream>

int main()
{
    std::cout << "Stir the soup 5 times.\n";
    for (int stir = 1; stir < 5; stir = stir + 1) {
        std::cout << "Stir number " << stir << '\n';
    }
    std::cout << "Finished stirring.\n";
    return 0;
}
