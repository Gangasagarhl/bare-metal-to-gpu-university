// A tiny grid-world robot simulator written for KID101 (chapters F0-08 and F0-10).
// The robot starts at column 0, row 0, facing east. Commands, one per line:
// FORWARD (one square), LEFT (turn a quarter left), RIGHT (turn a quarter right).
// Rows grow towards the south. This is our own teaching model.
#include <iostream>
#include <string>
#include <vector>

inline int runRobot(std::istream& in)
{
    const std::vector<std::string> facingNames = {"east", "south", "west", "north"};
    const std::vector<int> stepColumn = {1, 0, -1, 0};
    const std::vector<int> stepRow = {0, 1, 0, -1};
    int column = 0;
    int row = 0;
    int facing = 0;
    std::string command;
    int number = 0;
    std::cout << "start: column " << column << ", row " << row << ", facing " << facingNames[facing] << '\n';
    while (in >> command) {
        number = number + 1;
        if (command == "FORWARD") {
            column = column + stepColumn[facing];
            row = row + stepRow[facing];
        } else if (command == "RIGHT") {
            facing = (facing + 1) % 4;
        } else if (command == "LEFT") {
            facing = (facing + 3) % 4;
        } else {
            std::cout << "step " << number << ": unknown command " << command << " (ignored)\n";
            continue;
        }
        std::cout << "step " << number << ": " << command << " -> column " << column << ", row "
                  << row << ", facing " << facingNames[facing] << '\n';
    }
    std::cout << "end: column " << column << ", row " << row << '\n';
    return 0;
}
