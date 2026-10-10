#include <iostream>

void taste(const int& spoons_of_salt)
{
    spoons_of_salt = 0;                  // not allowed: the parameter is const
    std::cout << "Tasting with " << spoons_of_salt << " spoons\n";
}

int main()
{
    int salt = 2;
    taste(salt);
    return 0;
}
