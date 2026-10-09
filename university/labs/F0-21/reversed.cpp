// Evidence program for the forensic lab: Priya's first converter.
#include <iostream>
#include <string>

std::string toBinary(int number)
{
    std::string bits;
    while (number > 0) {
        bits = bits + std::to_string(number % 2);
        number = number / 2;
    }
    return bits;
}

int main()
{
    for (int n : {1, 2, 3, 5, 6, 9, 12, 13}) {
        std::cout << n << " -> " << toBinary(n) << '\n';
    }
    return 0;
}
