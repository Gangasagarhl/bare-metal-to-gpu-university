// Count from 0 to 15 and show each number as four switches (bits).
#include <bitset>
#include <iostream>

int main()
{
    for (int number = 0; number <= 15; ++number) {
        std::bitset<4> switches(number);
        std::cout << number << " = " << switches << '\n';
    }
    return 0;
}
