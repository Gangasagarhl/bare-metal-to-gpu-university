// Evidence program for the forensic lab: Sam's counter with a row of switches.
#include <bitset>
#include <iostream>

int main()
{
    for (int number = 0; number <= 10; ++number) {
        std::bitset<3> switches(number);
        std::cout << number << " = " << switches << '\n';
    }
    return 0;
}
