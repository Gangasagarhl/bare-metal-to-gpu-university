// Forensic evidence for F0-19: a bounds check with one wrong symbol (deliberate).
// It only counts, so the mistake shows up as a wrong total instead of a crash.
#include <iostream>

int main()
{
    int jobs = 10;
    int teamSize = 4;
    int teams = (jobs + teamSize - 1) / teamSize;
    int jobsDone = 0;

    for (int team = 0; team < teams; ++team) {
        for (int seat = 0; seat < teamSize; ++seat) {
            int ticket = team * teamSize + seat;
            if (ticket <= jobs) {
                ++jobsDone;
                std::cout << "helper ticket " << ticket << " works on job " << ticket << "\n";
            } else {
                std::cout << "helper ticket " << ticket << " has no job\n";
            }
        }
    }
    std::cout << "jobs done: " << jobsDone << " (the list has " << jobs << " jobs, numbered 0 to 9)\n";
    return 0;
}
