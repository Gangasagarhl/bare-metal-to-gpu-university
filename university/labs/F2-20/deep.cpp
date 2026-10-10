#include <array>
#include <iostream>

// Forensic evidence: a recursive count that works for small menus and crashes on big ones.
int countDishes(int remaining)
{
    std::array<char, 1024> notes{};          // 1 KiB of scratch space in every frame
    notes[0] = static_cast<char>(remaining);
    if (remaining == 0) {
        return notes[0];
    }
    return 1 + countDishes(remaining - 1) + notes[0] * 0;
}

int main()
{
    std::cout << std::unitbuf;
    for (const int n : {10, 1000, 100000}) {
        std::cout << "menu of " << n << " dishes: ";
        std::cout << countDishes(n) << " counted\n";
    }
    return 0;
}
