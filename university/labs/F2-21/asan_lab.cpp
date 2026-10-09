#include <iostream>
#include <string>
#include <vector>

// Lab: this program has ONE memory bug. Run it, read the report, fix the line it names.
struct Guest
{
    std::string name;
    int seat;
};

int main()
{
    std::cout << std::unitbuf;
    std::vector<Guest*> guests;
    guests.push_back(new Guest{"Ana", 1});
    guests.push_back(new Guest{"Ravi", 2});
    guests.push_back(new Guest{"Mei", 3});

    for (Guest* g : guests) {
        std::cout << g->name << " sits in seat " << g->seat << '\n';
    }
    for (Guest* g : guests) {           // the evening is over: release every guest record
        delete g;
    }
    std::cout << "last guest sat in seat " << guests.back()->seat << '\n';
    return 0;
}
