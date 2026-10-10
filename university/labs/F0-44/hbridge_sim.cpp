// A motor driver drawn as four switches (an "H-bridge" truth model).
//   S1 = top-left,  S2 = bottom-left,  S3 = top-right,  S4 = bottom-right.
// The motor sits in the middle, between the left side and the right side.
// Each input line: step name, then S1 S2 S3 S4 (1 = closed, 0 = open).
#include <iostream>
#include <string>

int main()
{
    std::string step;
    int s1 = 0;
    int s2 = 0;
    int s3 = 0;
    int s4 = 0;

    while (std::cin >> step >> s1 >> s2 >> s3 >> s4) {
        std::cout << step << ": S1=" << s1 << " S2=" << s2 << " S3=" << s3 << " S4=" << s4
                  << " -> ";
        if ((s1 == 1 && s2 == 1) || (s3 == 1 && s4 == 1)) {
            std::cout << "SHORT CIRCUIT: battery + joined straight to battery - (never allowed)\n";
        } else if (s1 == 1 && s4 == 1) {
            std::cout << "current flows left to right: motor turns FORWARD\n";
        } else if (s3 == 1 && s2 == 1) {
            std::cout << "current flows right to left: motor turns BACKWARD\n";
        } else {
            std::cout << "no path through the motor: motor not driven\n";
        }
    }
    return 0;
}
