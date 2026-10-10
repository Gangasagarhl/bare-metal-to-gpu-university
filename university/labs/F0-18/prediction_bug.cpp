// Forensic evidence for F0-18: a prediction for the toy robot with one wrong step (deliberate).
#include <iostream>

int main()
{
    int start = 10;
    int speed = 15;
    int t = 6;

    int step1 = speed * t;
    int step2 = start + step1;
    int step3 = step2 + start;  // the start was added a second time

    std::cout << "Predict the toy robot's distance at t = 6 s (rule: d = 10 + 15t)\n";
    std::cout << "step 1: 15 x 6 = " << step1 << "\n";
    std::cout << "step 2: 10 + " << step1 << " = " << step2 << "\n";
    std::cout << "step 3: add the starting distance: " << step2 << " + 10 = " << step3 << "\n";
    std::cout << "prediction: " << step3 << " cm\n";
    std::cout << "simulator log at t = 6 s: " << start + speed * t << " cm\n";
    return 0;
}
