#include <iostream>
#include <stdexcept>

int main()
{
    std::cout << "about to throw, and nobody catches" << std::endl;
    throw std::runtime_error("the oven timer is broken");
}
