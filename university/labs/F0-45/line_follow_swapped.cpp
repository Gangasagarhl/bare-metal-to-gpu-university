// Forensic evidence: the same robot, but the left and right sensor wires were swapped.
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
    int col = 4;

    for (std::size_t row = 0; row < track.size(); ++row) {
        // BUG (deliberate): left reads the right-hand cell and right reads the left-hand cell.
        const bool left = col + 1 < width && track[row][col + 1] == '#';
        const bool middle = track[row][col] == '#';
        const bool right = col > 0 && track[row][col - 1] == '#';

        std::string action = "straight";
        if (!middle && left) {
            col -= 1;
            action = "steer left";
        } else if (!middle && right) {
            col += 1;
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
