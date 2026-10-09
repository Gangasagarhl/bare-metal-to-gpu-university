#include <iostream>

int main()
{
    for (char letter = 'A'; letter <= 'Z'; ++letter) {
        std::cout << letter << " = " << static_cast<int>(letter) << '\n';
    }
    return 0;
}
