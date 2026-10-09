#include <iostream>

int main()
{
    int slices = 3;
    int friends = 2;
    double each = static_cast<double>(slices) / friends;
    std::cout << "Slices: " << slices << ", friends: " << friends << '\n';
    std::cout << "Each friend gets " << each << " slices\n";
    return 0;
}
