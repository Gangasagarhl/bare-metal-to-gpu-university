// Lab program (F0-19): plan teams of helpers for any number of jobs, read from the input.
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

    int teams = (jobs + teamSize - 1) / teamSize;
    int helpers = teams * teamSize;
    int working = 0;
    for (int ticket = 0; ticket < helpers; ++ticket) {
        if (ticket < jobs) {
            ++working;
        }
    }
    std::cout << "jobs: " << jobs << ", team size: " << teamSize << "\n";
    std::cout << "teams: " << teams << ", helpers: " << helpers << "\n";
    std::cout << "working: " << working << ", idle: " << helpers - working << "\n";
    std::cout << "allowed tickets: 0 <= ticket < " << jobs << "\n";
    return 0;
}
