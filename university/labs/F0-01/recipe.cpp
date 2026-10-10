#include <iostream>
#include <string>
#include <vector>

int main()
{
    const std::vector<std::string> steps = {
        "Open the bread bag.",
        "Take one slice and put it on the plate.",
        "Take a second slice and put it next to the first one.",
        "Spread butter on the first slice.",
        "Put one piece of cheese on the buttered slice.",
        "Put the second slice on top of the cheese.",
    };

    int number = 1;
    for (const std::string& step : steps) {
        std::cout << "Step " << number << ": " << step << '\n';
        number = number + 1;
    }
    std::cout << "Finished: " << steps.size() << " steps, done in order.\n";
    return 0;
}
