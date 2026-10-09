#include <iostream>

struct Tray
{
    int cups;
};

void clearTable(Tray* t)
{
    delete t;                           // this function thinks it owns the tray
}

int main()
{
    std::cout << std::unitbuf;
    Tray* tray = new Tray{4};
    std::cout << "tray has " << tray->cups << " cups\n";
    clearTable(tray);
    std::cout << "table cleared\n";
    delete tray;                        // ...and so does main: freed twice
    return 0;
}
