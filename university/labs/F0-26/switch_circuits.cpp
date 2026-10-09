// The university's tiny text model of light circuits with two switches, A and B.
// true means a switch is pressed (closed) or the lamp is lit; false means open or dark.
#include <iostream>

bool seriesCircuit(bool a, bool b)    // A and B one after the other: AND
{
    return a && b;
}

bool parallelCircuit(bool a, bool b)  // A and B side by side: OR
{
    return a || b;
}

bool stairCircuit(bool a, bool b)     // two-way switches at the bottom and top of a stair: XOR
{
    return a != b;
}

bool pushToBreak(bool a)              // a button that breaks the circuit when pressed: NOT
{
    return !a;
}

int main()
{
    std::cout << "A B | AND OR XOR\n";
    for (bool a : {false, true}) {
        for (bool b : {false, true}) {
            std::cout << a << ' ' << b << " |  " << seriesCircuit(a, b) << "   "
                      << parallelCircuit(a, b) << "   " << stairCircuit(a, b) << '\n';
        }
    }
    std::cout << "\nA | NOT\n";
    for (bool a : {false, true}) {
        std::cout << a << " |  " << pushToBreak(a) << '\n';
    }
    return 0;
}
