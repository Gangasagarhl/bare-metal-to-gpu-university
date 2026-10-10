#include <algorithm>
#include <iostream>
#include <string>

// Does not compile with the course flags: a reference to a temporary that is about to die.
// (std::max compares strings alphabetically, so "later" means later in the alphabet.)
int main()
{
    const std::string& later = std::max(std::string("rice"), std::string("noodles"));
    std::cout << later << '\n';
    return 0;
}
