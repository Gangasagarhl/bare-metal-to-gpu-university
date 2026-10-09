#include <iostream>
#include <string>

#include "ring_bug.h"

// Forensic evidence: replays the evening's tickets through the rail and logs what the cooks got.
int main()
{
    OrderRail<int, 4> rail;
    int ticket = 0;
    int next = 101;
    std::string served;
    // Quiet start: two tickets in, two served.
    for (int i = 0; i < 2; ++i) {
        rail.push(next++);
    }
    for (int i = 0; i < 2; ++i) {
        rail.pop(ticket);
        served += std::to_string(ticket) + " ";
    }
    std::cout << "18:00 served: " << served << '\n';
    // The rush: four tickets in, then all served.
    served = "";
    std::string in;
    for (int i = 0; i < 4; ++i) {
        in += std::to_string(next) + " ";
        rail.push(next++);
    }
    while (rail.pop(ticket)) {
        served += std::to_string(ticket) + " ";
    }
    std::cout << "19:00 tickets in: " << in << '\n';
    std::cout << "19:00 served:     " << served << '\n';
    return 0;
}
