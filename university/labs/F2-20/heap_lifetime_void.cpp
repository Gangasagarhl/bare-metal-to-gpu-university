#include <iostream>
#include <memory>
#include <string>

struct Ticket
{
    std::string dish;
    Ticket(std::string d) : dish(std::move(d)) { std::cout << "  ticket for " << dish << " made\n"; }
    ~Ticket() { std::cout << "  ticket for " << dish << " thrown away\n"; }
};

// Check-yourself 5: the same function, but the heap ticket is not returned.
void takeOrder(const std::string& dish)
{
    Ticket onStack("tea (stack)");
    auto onHeap = std::make_unique<Ticket>(dish + " (heap)");
    std::cout << "  leaving takeOrder\n";
    // nothing is returned: onHeap still owns the heap ticket and ends here
}

int main()
{
    std::cout << "main calls takeOrder\n";
    takeOrder("soup");
    std::cout << "back in main, holding nothing\n";
    std::cout << "main ends\n";
    return 0;
}
