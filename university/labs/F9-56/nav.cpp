// nav.cpp - F9-56 lab: navigate to goals with the mini navigation stack.
// The configuration (parameters, goals, obstacles missing from the map) comes from nav.in.
#include "nav.hpp"

int main()
{
    return rb::runNavigation(std::cin) == 0 ? 0 : 3;
}
