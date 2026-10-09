// Fixed version for the forensic lab answer key.
// Each line is one moment: someone flips one switch, and the model prints the lamp.
#include <iostream>
#include <string>

bool lamp(bool bottom, bool top)
{
    return bottom != top;  // fixed: XOR, as in the stair circuit
}

int main()
{
    bool bottom = false;
    bool top = false;
    const std::string flips = "BTBTTB";  // B = flip the bottom switch, T = flip the top switch
    std::cout << "start          bottom=" << bottom << " top=" << top
              << " lamp=" << lamp(bottom, top) << '\n';
    for (char flip : flips) {
        if (flip == 'B') {
            bottom = !bottom;
        } else {
            top = !top;
        }
        std::cout << "flip " << flip << "         bottom=" << bottom << " top=" << top
                  << " lamp=" << lamp(bottom, top) << '\n';
    }
    return 0;
}
