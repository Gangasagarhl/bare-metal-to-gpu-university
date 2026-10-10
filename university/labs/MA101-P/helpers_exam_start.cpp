// MA101 practical exam - starting file. Complete the three TODO lines, then build and run.
// Build: g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined helpers_exam_start.cpp -o plan
// Run:   ./plan < helpers_exam_start.in
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

    int teams = 0;    // TODO 1: round up, so that teams * teamSize is at least jobs (F0-12)
    int helpers = teams * teamSize;

    int working = 0;
    int idle = 0;
    int lastWorkingTicket = -1;
    int firstIdleTicket = -1;
    for (int team = 0; team < teams; ++team) {
        for (int seat = 0; seat < teamSize; ++seat) {
            int ticket = 0;    // TODO 2: the ticket formula (F0-16)
            if (ticket < 0) {  // TODO 3: the bounds check: work only when the ticket is in 0 <= ticket < jobs (F0-19)
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
