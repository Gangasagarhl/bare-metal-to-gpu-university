// MA101 final exam, forensic question: evidence program with one deliberate mistake (Lab Engineer).
// A family shares 50 jobs (numbered 0 to 49) among teams of 8 helpers.
#include <iostream>
#include <vector>

int main()
{
    int jobs = 50;
    int teamSize = 8;
    int teams = jobs / teamSize;
    std::cout << "Plan: " << jobs << " jobs, teams of " << teamSize << "\n";
    std::cout << "step 1: teams = " << jobs << " / " << teamSize << " = " << teams << "\n";
    std::cout << "step 2: helpers = " << teams << " x " << teamSize << " = " << teams * teamSize << "\n";

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
    std::cout << "step 3: jobs never done:";
    for (int i = 0; i < jobs; ++i) {
        finished = finished + done[i];
        if (done[i] == 0) {
            std::cout << " " << i;
        }
    }
    std::cout << "\n";
    std::cout << "step 4: jobs done: " << finished << " of " << jobs << ", idle helpers: " << idle << "\n";
    return 0;
}
