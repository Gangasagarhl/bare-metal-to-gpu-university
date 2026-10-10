// show_maps.cc - print this process's own address-space map, as the Linux kernel reports it.
#include <fstream>
#include <iostream>
#include <string>

int global = 7;

void someCode() {}

int main()
{
    int local = 0;
    std::cout << "address of a global:  " << static_cast<void*>(&global) << "\n";
    std::cout << "address of a local:   " << static_cast<void*>(&local) << "\n";
    std::cout << "address of code:      " << reinterpret_cast<void*>(&someCode) << "\n";
    std::ifstream maps("/proc/self/maps");
    std::string line;
    while (std::getline(maps, line)) { std::cout << line << "\n"; }
    return 0;
}
