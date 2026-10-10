#include <iostream>

// Writes past the end of a built-in array on the stack.
int main()
{
    std::cout << std::unitbuf;
    int seats[4] = {0, 0, 0, 0};
    int guestsWaiting = 2;
    for (int i = 0; i <= 4; ++i) {                       // the fifth write lands outside
        seats[i] = 1;
    }
    std::cout << "seat 0 taken: " << seats[0] << ", guests waiting: " << guestsWaiting << '\n';
    return 0;
}
