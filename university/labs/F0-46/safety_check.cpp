// A pre-build safety checklist. It reads one answer (y or n) per question
// and refuses to say "go" unless every answer is y.
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
            answer = "n"; // no answer counts as no
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
