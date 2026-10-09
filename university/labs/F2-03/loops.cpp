#include <iostream>
#include <vector>

int main()
{
    // for: count with a counter.
    for (int table = 1; table <= 5; ++table) {
        if (table == 3) {
            continue;                    // table 3 is closed tonight: skip it
        }
        std::cout << "Set table " << table << '\n';
    }

    // while: repeat while a condition holds; it may run zero times.
    int pancakes = 3;
    while (pancakes > 0) {
        std::cout << "Flip a pancake, " << pancakes << " to go\n";
        --pancakes;
    }

    // do-while: the body runs once before the condition is checked.
    int tastes = 0;
    do {
        ++tastes;
        std::cout << "Taste number " << tastes << '\n';
    } while (tastes < 1);

    // range-for: visit every element of a container.
    std::vector<int> minutes{4, 9, 2};
    int total = 0;
    for (int m : minutes) {
        total += m;
    }
    std::cout << "Total cooking time: " << total << " minutes\n";
    return 0;
}
