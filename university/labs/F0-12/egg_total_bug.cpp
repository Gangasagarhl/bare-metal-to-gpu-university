// Forensic evidence for F0-12: an egg count with one wrong step (deliberate).
#include <iostream>

int main()
{
    int trays = 3;
    int eggsPerTray = 12;
    int looseEggs = 5;

    int step1 = eggsPerTray + looseEggs;
    int step2 = trays * step1;

    std::cout << "Egg count for the class bake\n";
    std::cout << "question: " << trays << " trays of " << eggsPerTray << " eggs, plus " << looseEggs
              << " loose eggs\n";
    std::cout << "step 1: " << eggsPerTray << " + " << looseEggs << " = " << step1 << "\n";
    std::cout << "step 2: " << trays << " x " << step1 << " = " << step2 << "\n";
    std::cout << "total eggs: " << step2 << "\n";
    return 0;
}
