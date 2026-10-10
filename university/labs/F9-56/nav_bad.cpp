// nav_bad.cpp - F9-56 forensic evidence: the same stack, run with the configuration in nav_bad.in.
// The configuration (parameters, goals, obstacles missing from the map) comes from nav_bad.in.
#include "nav.hpp"

int main()
{
    return rb::runNavigation(std::cin) == 0 ? 0 : 3;
}
