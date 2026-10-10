// Listing 1 (F0-19): every helper checks "is my ticket in the allowed range?"
#include <iostream>
#include <vector>

int main()
{
    int jobs = 10;
    int teamSize = 4;
    int teams = (jobs + teamSize - 1) / teamSize;

    std::vector<int> done(jobs, 0);
    int idle = 0;

    for (int team = 0; team < teams; ++team) {
        for (int seat = 0; seat < teamSize; ++seat) {
            int ticket = team * teamSize + seat;
            if (ticket < jobs) {
                done[ticket] = 1;
            } else {
                ++idle;
            }
        }
    }

    int finished = 0;
    for (int d : done) {
        finished = finished + d;
    }
    std::cout << "teams: " << teams << ", helpers: " << teams * teamSize << "\n";
    std::cout << "jobs done: " << finished << " of " << jobs << ", idle helpers: " << idle << "\n";
    return 0;
}
