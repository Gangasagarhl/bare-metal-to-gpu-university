// ask_name.cpp - the "frozen" tool of the forensic lab: it waits for a line of input.
#include <iostream>
#include <string>

int main()
{
    std::cout << "Name of the class to print the timetable for: " << std::flush;
    std::string name;
    if (!std::getline(std::cin, name)) {
        std::cout << "\nno input\n";
        return 1;
    }
    std::cout << "Timetable for " << name << ": (empty)\n";
    return 0;
}
