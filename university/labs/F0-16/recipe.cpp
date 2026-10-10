// Listing 1 (F0-16): variables, expressions and the seat-number formula.
#include <iostream>

int main()
{
    int batches = 3;
    int eggs = 2 * batches;
    int flour = 150 * batches;
    std::cout << "batches: " << batches << ", eggs: " << eggs << ", flour (g): " << flour << "\n";

    int seatsPerTable = 4;
    int table = 2;
    int seat = 3;
    int ticket = table * seatsPerTable + seat;
    std::cout << "table " << table << ", seat " << seat << " has ticket " << ticket << "\n";
    return 0;
}
