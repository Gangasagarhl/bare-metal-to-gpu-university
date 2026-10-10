// MA101 practical exam - reference solution (Lab Engineer). The three TODO lines are completed.
// Build: g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined helpers_exam.cpp -o plan
// Run:   ./plan < helpers_exam.in
#include <iostream>

int main()
{
    int jobs = 0;
    int teamSize = 0;
    std::cin >> jobs >> teamSize;
    if (jobs < 1 || teamSize < 1) {
        std::cout << "jobs and team size must both be at least 1\n";
        return 1;
    }

    int teams = (jobs + teamSize - 1) / teamSize;   // rounded up (F0-12)
    int helpers = teams * teamSize;

    int working = 0;
    int idle = 0;
    int lastWorkingTicket = -1;
    int firstIdleTicket = -1;
    for (int team = 0; team < teams; ++team) {
        for (int seat = 0; seat < teamSize; ++seat) {
            int ticket = team * teamSize + seat;             // the ticket formula (F0-16)
            if (ticket < jobs) {                             // the bounds check (F0-19)
                ++working;
                lastWorkingTicket = ticket;
            } else {
                ++idle;
                if (firstIdleTicket < 0) {
                    firstIdleTicket = ticket;
                }
            }
        }
    }

    std::cout << "jobs: " << jobs << ", team size: " << teamSize << "\n";
    std::cout << "teams: " << teams << ", helpers: " << helpers << "\n";
    std::cout << "working: " << working << ", idle: " << idle << "\n";
    std::cout << "allowed tickets: 0 <= ticket < " << jobs << "\n";
    if (lastWorkingTicket >= 0) {
        std::cout << "last working ticket: " << lastWorkingTicket << " (team " << lastWorkingTicket / teamSize
                  << ", seat " << lastWorkingTicket % teamSize << ")\n";
    } else {
        std::cout << "last working ticket: none\n";
    }
    std::cout << "first idle ticket: " << firstIdleTicket << " (-1 means no helper is idle)\n";
    return 0;
}
