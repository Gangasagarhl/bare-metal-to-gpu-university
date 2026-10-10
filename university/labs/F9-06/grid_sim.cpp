// F9-06 Listing 1: read robot programs (one per line) and run each on the course.
#include <iostream>
#include <string>
#include "grid_world.hpp"

int main()
{
    std::string commands;
    while (std::getline(std::cin, commands)) {
        runCourse(commands);
    }
    return 0;
}
