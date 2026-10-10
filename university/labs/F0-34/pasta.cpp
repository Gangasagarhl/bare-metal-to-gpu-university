#include <iostream>
#include <string>

void boil_water()
{
    std::cout << "Boil a big pot of water.\n";
}

void add_to_pot(std::string thing)
{
    std::cout << "Add the " << thing << " to the pot.\n";
}

int double_it(int number)
{
    return number * 2;
}

int main()
{
    boil_water();
    add_to_pot("salt");
    add_to_pot("pasta");
    int plates = double_it(3);
    std::cout << "Make enough for " << plates << " plates.\n";
    return 0;
}
