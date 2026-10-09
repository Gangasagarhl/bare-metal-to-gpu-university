#include <iostream>
#include <memory>
#include <string>

struct Ticket
{
    std::string dish;
    Ticket(std::string d) : dish(std::move(d)) { std::cout << "  ticket for " << dish << " made\n"; }
    ~Ticket() { std::cout << "  ticket for " << dish << " thrown away\n"; }
};

// A stack object ends at its closing brace; a heap object ends when its owner lets go.
std::unique_ptr<Ticket> takeOrder(const std::string& dish)
{
    Ticket onStack("tea (stack)");
    auto onHeap = std::make_unique<Ticket>(dish + " (heap)");
    std::cout << "  leaving takeOrder\n";
    return onHeap;                     // ownership of the heap ticket moves to the caller
}

int main()
{
    std::cout << "main calls takeOrder\n";
    auto kept = takeOrder("soup");
    std::cout << "back in main, still holding: " << kept->dish << '\n';
    std::cout << "main ends\n";
    return 0;
}
