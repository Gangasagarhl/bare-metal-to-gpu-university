#include <iostream>
#include <string>
#include <vector>

// Lab answer: read what you need BEFORE releasing the records (one possible fix).
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
    const int lastSeat = guests.back()->seat;   // read while the record is still alive
    for (Guest* g : guests) {           // the evening is over: release every guest record
        delete g;
    }
    std::cout << "last guest sat in seat " << lastSeat << '\n';
    return 0;
}
