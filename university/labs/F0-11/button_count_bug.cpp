// Forensic evidence for F0-11: a counting helper with one wrong step (deliberate).
#include <iostream>

int main()
{
    int bowls = 4;  // bowls of one hundred buttons
    int cups = 0;   // cups of ten buttons
    int loose = 6;  // single buttons

    int step1 = bowls * 10;
    int step2 = cups * 10;
    int step3 = loose;
    int total = step1 + step2 + step3;

    std::cout << "Counting sheet for the button jar\n";
    std::cout << "step 1: bowls of one hundred: " << bowls << " -> " << bowls << " x 10 = " << step1 << "\n";
    std::cout << "step 2: cups of ten:          " << cups << " -> " << cups << " x 10 = " << step2 << "\n";
    std::cout << "step 3: loose buttons:        " << loose << " -> " << step3 << "\n";
    std::cout << "step 4: total = " << step1 << " + " << step2 << " + " << step3 << " = " << total << "\n";
    return 0;
}
