#include <iostream>
#include <string>

struct Bill
{
    int table;
    double total;
};

// Check-yourself 5: the early return now deletes, but the normal path forgot.
double printBill(int table, double total)
{
    Bill* bill = new Bill{table, total};
    if (bill->total <= 0.0) {
        std::cout << "table " << table << ": nothing to pay\n";
        delete bill;
        return 0.0;
    }
    std::cout << "table " << table << ": pay " << bill->total << '\n';
    const double result = bill->total;
    return result;                      // leak: bill is never deleted on this path
}

int main()
{
    std::cout << std::unitbuf;   // print each line at once: the leak report ends the program
    printBill(3, 12.5);
    printBill(4, 0.0);
    printBill(5, 8.0);
    return 0;
}
