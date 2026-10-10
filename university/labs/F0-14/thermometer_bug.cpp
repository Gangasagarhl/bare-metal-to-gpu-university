// Forensic evidence for F0-14: a temperature change worked out with one wrong step (deliberate).
#include <iostream>

int main()
{
    int morning = -3;
    int afternoon = 5;

    int morningUsed = 3;  // the sign was dropped when the number was copied
    int change = afternoon - morningUsed;

    std::cout << "Garden thermometer, change from morning to afternoon\n";
    std::cout << "step 1: morning reading:   " << morning << "\n";
    std::cout << "step 2: afternoon reading: " << afternoon << "\n";
    std::cout << "step 3: change = afternoon - morning = " << afternoon << " - " << morningUsed << "\n";
    std::cout << "step 4: change = " << change << " degrees\n";
    return 0;
}
