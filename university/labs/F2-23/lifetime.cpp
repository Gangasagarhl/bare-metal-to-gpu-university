#include <iostream>
#include <string>

struct Plate
{
    std::string name;
    explicit Plate(std::string n) : name(std::move(n)) { std::cout << "  begin " << name << '\n'; }
    ~Plate() { std::cout << "  end   " << name << '\n'; }
};

struct Table
{
    Plate first{"table.first"};       // members begin in declaration order...
    Plate second{"table.second"};     // ...and end in reverse order
};

Plate makePlate(const std::string& n)
{
    return Plate(n);
}

int main()
{
    std::cout << "1. block scope\n";
    {
        Plate a("a");
        Plate b("b");
        std::cout << "  (end of block)\n";
    }
    std::cout << "2. a temporary lives until the end of the full expression\n";
    std::cout << "  length " << makePlate("temp").name.size() << '\n';
    std::cout << "3. binding a temporary to a const reference extends its life\n";
    {
        const Plate& kept = makePlate("kept");
        std::cout << "  still alive: " << kept.name << '\n';
    }
    std::cout << "4. members\n";
    {
        Table t;
    }
    std::cout << "5. done\n";
    return 0;
}
