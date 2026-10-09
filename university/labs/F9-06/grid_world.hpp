// F9-06 Listing 2: the university's tiny grid-world robot simulator.
#pragma once
#include <array>
#include <iostream>
#include <string>
#include <vector>

// Runs one robot program on the course and prints what happens.
// Commands: F = forward one square, L = turn left, R = turn right.
inline void runCourse(const std::string& commands)
{
    std::vector<std::string> map = {
        "#########",
        "#S..#...#",
        "###.#.#.#",
        "#...#.#.#",
        "#.###.#.#",
        "#.....#G#",
        "#########",
    };
    const std::array<int, 4> moveColumn = {1, 0, -1, 0};   // east, south, west, north
    const std::array<int, 4> moveRow = {0, 1, 0, -1};
    const std::array<std::string, 4> name = {"east", "south", "west", "north"};

    int row = 1;
    int column = 1;
    int facing = 0;                       // the robot starts facing east
    int step = 0;
    std::string result = "program ended before the goal";

    for (char command : commands) {
        step = step + 1;
        if (command == 'L') {
            facing = (facing + 3) % 4;    // turning left = three right turns
        } else if (command == 'R') {
            facing = (facing + 1) % 4;
        } else if (command == 'F') {
            int nextRow = row + moveRow[facing];
            int nextColumn = column + moveColumn[facing];
            char ahead = map[nextRow][nextColumn];   // SENSE: what is in front?
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
