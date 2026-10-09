#include <cassert>
#include <iostream>

constexpr int cups_per_guest = 2;
static_assert(cups_per_guest > 0, "every guest needs at least one cup");

int cups_needed(int guests)
{
    assert(guests >= 0);                 // a precondition: nobody orders for -3 guests
    return guests * cups_per_guest + 1;  // one spare cup
}

int main()
{
    assert(cups_needed(0) == 1);         // edge case: an empty table still gets a spare
    assert(cups_needed(1) == 3);
    assert(cups_needed(4) == 9);
    std::cout << "cups_needed: 3 checks passed\n";
    return 0;
}
