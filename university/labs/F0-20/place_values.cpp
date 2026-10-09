// Add up the place values of the switches that are on.
#include <iostream>
#include <string>

int main()
{
    const std::string row = "1011";  // the switches, left to right
    int value = 0;
    int placeValue = 1;              // the rightmost switch is worth 1
    for (int i = static_cast<int>(row.size()) - 1; i >= 0; --i) {
        if (row[i] == '1') {
            std::cout << "switch worth " << placeValue << " is on\n";
            value = value + placeValue;
        }
        placeValue = placeValue * 2;  // each switch is worth double the one to its right
    }
    std::cout << row << " in binary is " << value << " in decimal\n";
    return 0;
}
