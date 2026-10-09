#include <iostream>

#include "recipe.h"
#include "shopping.h"

int main()
{
    Recipe soup{"soup", 30};
    std::cout << soup.name << " takes " << soup.minutes << " minutes\n";
    return 0;
}
