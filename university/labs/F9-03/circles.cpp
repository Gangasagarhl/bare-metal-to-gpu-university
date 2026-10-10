// F9-03 forensic evidence: both motors are told "4 turns per step",
// but the simulator gives the left motor a fault (it is weaker).
#include <iomanip>
#include <iostream>

int main()
{
    const int commandLeft = 4;      // what the program asks the left motor
    const int commandRight = 4;     // what the program asks the right motor
    const int leftActual = 3;       // simulated fault: left wheel only manages 3
    const int rightActual = 4;
    int leftCount = 0;
    int rightCount = 0;

    std::cout << "step  command L/R  encoder L  encoder R\n";
    for (int step = 1; step <= 8; ++step) {
        leftCount = leftCount + leftActual;
        rightCount = rightCount + rightActual;
        std::cout << std::setw(4) << step << std::setw(10) << commandLeft << "/"
                  << commandRight << std::setw(11) << leftCount << std::setw(11)
                  << rightCount << '\n';
    }
    return 0;
}
