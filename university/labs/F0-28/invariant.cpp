// Converting 45 to binary while checking a sentence that must stay true at every step:
//   original == rest * placeValue + doneSoFar
#include <iostream>
#include <string>

int main()
{
    const int original = 45;
    int rest = original;   // what is still left to convert
    int placeValue = 1;    // the value of the next bit we will write
    int doneSoFar = 0;     // the value of the bits already written
    std::string bits;

    while (rest > 0) {
        const bool holds = (original == rest * placeValue + doneSoFar);
        std::cout << "rest=" << rest << " placeValue=" << placeValue
                  << " doneSoFar=" << doneSoFar << " bits=" << (bits.empty() ? "-" : bits)
                  << "  invariant " << (holds ? "holds" : "BROKEN") << '\n';
        const int bit = rest % 2;
        bits = std::to_string(bit) + bits;
        doneSoFar = doneSoFar + bit * placeValue;
        placeValue = placeValue * 2;
        rest = rest / 2;
    }
    const bool holds = (original == rest * placeValue + doneSoFar);
    std::cout << "rest=" << rest << " placeValue=" << placeValue << " doneSoFar=" << doneSoFar
              << " bits=" << bits << "  invariant " << (holds ? "holds" : "BROKEN") << '\n';
    std::cout << original << " = " << bits << " in binary\n";
    return 0;
}
