// A simulated robot follows a line drawn on a text grid.
// '#' is the line. The robot drives down one row per step. It has three
// sensors (left, middle, right) that look at the row it is on.
#include <iostream>
#include <string>
#include <vector>

int main()
{
    std::vector<std::string> track = {
        "....#.......",
        "....#.......",
        ".....#......",
        "......#.....",
        "......#.....",
        ".......#....",
        "......#.....",
        ".....#......",
        "....#.......",
        "....#.......",
    };
    const int width = static_cast<int>(track[0].size());
    int col = 4; // the robot starts on the line

    for (std::size_t row = 0; row < track.size(); ++row) {
        const bool left = col > 0 && track[row][col - 1] == '#';
        const bool middle = track[row][col] == '#';
        const bool right = col + 1 < width && track[row][col + 1] == '#';

        std::string action = "straight";
        if (!middle && left) {
            col -= 1;            // line is to the left: steer left
            action = "steer left";
        } else if (!middle && right) {
            col += 1;            // line is to the right: steer right
            action = "steer right";
        } else if (!middle) {
            action = "LOST the line";
        }

        std::string picture = track[row];
        picture[col] = 'R';
        std::cout << picture << "  sensors " << left << middle << right << "  " << action << "\n";
    }
    return 0;
}
