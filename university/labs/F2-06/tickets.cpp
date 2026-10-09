#include <iostream>
#include <string>
#include <vector>

// Version 2 of the ticket. In version 1 the first two fields were in the other order:
// int quantity; int table;
struct Ticket
{
    int table;
    int quantity;
    std::string dish;
};

void show(const Ticket& t)
{
    std::cout << "Table " << t.table << ": " << t.quantity << " x " << t.dish << '\n';
}

int main()
{
    // Order entry, written when version 1 was current: {quantity, table, dish}.
    std::vector<Ticket> tickets{
        {4, 2, "soup"},
        {1, 6, "noodles"},
        {2, 3, "tea"},
    };
    for (const Ticket& t : tickets) {
        show(t);
    }
    return 0;
}
