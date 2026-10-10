// RB101 practical: the grid-world course simulator (STARTING FILE: three TODO parts).
// Commands: F = forward one square, L = turn left, R = turn right. Start on S, facing east.
// The file builds and runs as it is, but it is wrong in three ways (see the TODO lines).
#include <array>
#include <iostream>
#include <string>
#include <vector>

void runCourse(const std::string& commands)
{
    std::vector<std::string> map = {
        "###########",
        "#S....#...#",
        "#.###.#.#.#",
        "#.#...#.#.#",
        "#.#.###.#.#",
        "#.#.....#G#",
        "###########",
    };
    const std::array<int, 4> moveColumn = {1, 0, -1, 0};   // east, south, west, north
    const std::array<int, 4> moveRow = {0, 1, 0, -1};
    const std::array<std::string, 4> name = {"east", "south", "west", "north"};
    const int maxSteps = 40;              // safety limit: a program may not run longer than this

    int row = 1;
    int column = 1;
    int facing = 0;                       // the robot starts facing east
    int step = 0;
    std::string result = "program ended before the goal";

    for (char command : commands) {
        step = step + 1;
        if (false) {                  // TODO 3: the safety limit (F9-04). Replace "false"
                                      //   with the question "is step above maxSteps?"
            result = "safety limit: stopped after " + std::to_string(maxSteps) +
                     " steps, goal not reached";
            break;
        }
        if (command == 'L') {
            facing = (facing + 1) % 4;    // TODO 2: this turns RIGHT. Make L turn left (F9-06).
        } else if (command == 'R') {
            facing = (facing + 1) % 4;
        } else if (command == 'F') {
            int nextRow = row + moveRow[facing];
            int nextColumn = column + moveColumn[facing];
            char ahead = '.';             // TODO 1: SENSE. Look at the map square in front
                                          //   instead of pretending it is floor (F9-06).
            if (ahead == '#') {
                result = "BUMP at step " + std::to_string(step) + ": wall ahead at row " +
                         std::to_string(nextRow) + ", column " + std::to_string(nextColumn) +
                         " while facing " + name[facing];
                break;
            }
            row = nextRow;
            column = nextColumn;
            if (ahead == 'G') {
                result = "GOAL reached at step " + std::to_string(step);
                break;
            }
            map[row][column] = '*';       // mark the path
        } else {
            result = "unknown command '" + std::string(1, command) + "' at step " +
                     std::to_string(step);
            break;
        }
    }
    std::cout << "program: " << commands << '\n';
    for (const std::string& line : map) {
        std::cout << "  " << line << '\n';
    }
    std::cout << "robot at row " << row << ", column " << column << ", facing "
              << name[facing] << '\n';
    std::cout << result << "\n\n";
}

int main()
{
    std::string commands;
    while (std::getline(std::cin, commands)) {
        runCourse(commands);
    }
    return 0;
}
