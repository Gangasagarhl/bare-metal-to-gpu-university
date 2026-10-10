#include <cassert>
#include <iostream>

int cups_needed(int guests)
{
    return guests * 2;                   // the spare cup was forgotten
}

int main()
{
    std::cout << "checking cups_needed...\n";
    assert(cups_needed(0) == 1);
    std::cout << "all checks passed\n";
    return 0;
}
