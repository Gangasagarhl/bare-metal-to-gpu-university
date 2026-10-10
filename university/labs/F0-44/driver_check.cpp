// Can a microcontroller pin run a motor by itself? PRETEND numbers only:
// real limits come from your kit's microcontroller and motor datasheets.
#include <iostream>

int main()
{
    const double pretendPinMaxMilliamps = 10.0;     // "pretend the board datasheet says 10 mA"
    const double pretendMotorNeedsMilliamps = 300.0; // "pretend the motor datasheet says 300 mA"

    const double ratio = pretendMotorNeedsMilliamps / pretendPinMaxMilliamps;
    std::cout << "pin can give at most: " << pretendPinMaxMilliamps << " mA\n";
    std::cout << "motor wants:          " << pretendMotorNeedsMilliamps << " mA\n";
    std::cout << "motor wants " << ratio << " times what the pin may give\n";
    if (pretendMotorNeedsMilliamps > pretendPinMaxMilliamps) {
        std::cout << "-> use a motor driver: the pin only sends the small 'go' signal\n";
    } else {
        std::cout << "-> still check the datasheets before connecting anything\n";
    }
    return 0;
}
