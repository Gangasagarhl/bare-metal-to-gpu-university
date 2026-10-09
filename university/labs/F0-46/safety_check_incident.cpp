// Forensic evidence: the same checklist, run with the answers recorded on the day
// of the "warm battery" incident. Same code as safety_check.cpp; different input.
#include <iostream>
#include <string>
#include <vector>

int main()
{
    const std::vector<std::string> questions = {
        "Is your adult in the room and watching?",
        "Is the battery pack disconnected while you change wires?",
        "Is the table clear of metal objects, drinks and loose parts?",
        "Are you using only the battery-powered kit chosen for this course?",
        "Has the circuit already worked in the simulator?",
        "Do you know to stop, hands off, and call your adult if anything gets warm or smells?",
    };

    int missing = 0;
    for (const std::string& question : questions) {
        std::string answer;
        if (!(std::cin >> answer)) {
            answer = "n";
        }
        const bool ok = (answer == "y");
        if (!ok) {
            ++missing;
        }
        std::cout << (ok ? "[yes] " : "[NO]  ") << question << "\n";
    }
    if (missing == 0) {
        std::cout << "All checks passed: you may start, with your adult watching.\n";
    } else {
        std::cout << "STOP: " << missing << " check(s) not passed. Fix them first.\n";
    }
    return 0;
}
