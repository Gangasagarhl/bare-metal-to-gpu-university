#include <iostream>
#include <string>

// Forensic evidence "The crash that moves". The kitchen screen keeps a raw pointer to the
// order it is showing. Closing an order deletes it, but nobody tells the screen.
struct Order
{
    int table;
    int portions;
    std::string dish;
};

struct Screen
{
    Order* showing = nullptr;
    void markDone() { showing->portions = 0; }        // writes through the stored pointer
};

int main()
{
    std::cout << std::unitbuf;
    Screen screen;

    Order* order = new Order{4, 2, "noodle soup"};
    screen.showing = order;
    std::cout << "open:  table " << order->table << ", " << order->portions << " x " << order->dish << '\n';
    delete order;                                       // order closed; the screen still points at it
    std::cout << "closed table 4\n";
#ifdef KITCHEN_DEBUG
    auto* log = new std::string("debug log: order for table 4 closed at the pass");
#endif

    auto* note = new std::string("table 7: extra spicy, no peanuts");
    std::cout << "note:  " << *note << '\n';

    screen.markDone();                                  // meant for the closed order
    std::cout << "note again: " << *note << '\n';

    delete note;
#ifdef KITCHEN_DEBUG
    std::cout << *log << '\n';
    delete log;
#endif
    std::cout << "end of service\n";
    return 0;
}
