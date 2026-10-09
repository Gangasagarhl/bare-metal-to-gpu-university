// Forensic evidence for F0-15: a count of switch patterns with one wrong step (deliberate).
#include <iostream>

int main()
{
    int switches = 5;
    int patterns = 2 * switches;  // the power was worked out as a multiplication

    std::cout << "How many on/off patterns can 5 light switches make?\n";
    std::cout << "step 1: each switch has 2 positions\n";
    std::cout << "step 2: there are " << switches << " switches\n";
    std::cout << "step 3: patterns = 2^" << switches << " = 2 x " << switches << " = " << patterns << "\n";
    std::cout << "answer: " << patterns << " patterns\n";
    return 0;
}
