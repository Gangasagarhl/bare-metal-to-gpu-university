// Forensic evidence for F0-13: a percentage worked out with one wrong step (deliberate).
#include <iostream>

int main()
{
    double grapes = 80.0;
    double percent = 25.0;

    double step1 = grapes / percent;
    double left = grapes - step1;

    std::cout << "Sharing grapes: 25 % of 80 grapes go to the neighbours\n";
    std::cout << "step 1: 25 % of 80 = 80 / 25 = " << step1 << "\n";
    std::cout << "step 2: grapes left = 80 - " << step1 << " = " << left << "\n";
    return 0;
}
