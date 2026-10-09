#include <iostream>
#include <memory>

struct Bill
{
    int table;
    double total;
};

// The same function with an owner object: every path frees the bill.
double printBill(int table, double total)
{
    auto bill = std::make_unique<Bill>(Bill{table, total});
    if (bill->total <= 0.0) {
        std::cout << "table " << table << ": nothing to pay\n";
        return 0.0;                     // bill's destructor frees the memory here too
    }
    std::cout << "table " << table << ": pay " << bill->total << '\n';
    return bill->total;
}

int main()
{
    printBill(3, 12.5);
    printBill(4, 0.0);
    printBill(5, 8.0);
    return 0;
}
