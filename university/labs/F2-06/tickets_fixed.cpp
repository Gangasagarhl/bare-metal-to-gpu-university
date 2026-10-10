#include <iostream>
#include <string>
#include <vector>

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
    // Designated initializers: every value is tied to a field name.
    std::vector<Ticket> tickets{
        {.table = 2, .quantity = 4, .dish = "soup"},
        {.table = 6, .quantity = 1, .dish = "noodles"},
        {.table = 3, .quantity = 2, .dish = "tea"},
    };
    for (const Ticket& t : tickets) {
        show(t);
    }
    return 0;
}
