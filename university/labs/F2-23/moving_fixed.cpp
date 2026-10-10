#include <iostream>
#include <memory>
#include <string>

// One fix for the forensic lab: the screen only OBSERVES the order through a weak_ptr,
// so it can tell when the order has been closed.
struct Order
{
    int table;
    int portions;
    std::string dish;
};

struct Screen
{
    std::weak_ptr<Order> showing;
    void markDone()
    {
        if (auto order = showing.lock()) {
            order->portions = 0;
        } else {
            std::cout << "screen: that order is already closed\n";
        }
    }
};

int main()
{
    Screen screen;
    auto order = std::make_shared<Order>(Order{4, 2, "noodle soup"});
    screen.showing = order;
    std::cout << "open:  table " << order->table << ", " << order->portions << " x " << order->dish << '\n';
    order.reset();                                      // order closed
    std::cout << "closed table 4\n";

    auto note = std::make_unique<std::string>("table 7: extra spicy, no peanuts");
    std::cout << "note:  " << *note << '\n';
    screen.markDone();
    std::cout << "note again: " << *note << '\n';
    std::cout << "end of service\n";
    return 0;
}
