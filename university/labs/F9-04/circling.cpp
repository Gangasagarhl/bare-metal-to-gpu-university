// F9-04 forensic evidence: a robot in an empty room with a flag in the middle.
// Its only rule: "if a wall is ahead, turn right; otherwise go forward".
#include <array>
#include <iostream>
#include <string>

int main()
{
    const int size = 5;                     // room squares 0..4 in each direction
    const int flagColumn = 2;
    const int flagRow = 2;
    const int maxSteps = 24;                // safety limit
    const std::array<int, 4> moveColumn = {1, 0, -1, 0};   // east, south, west, north
    const std::array<int, 4> moveRow = {0, 1, 0, -1};
    const std::array<std::string, 4> name = {"east", "south", "west", "north"};

    int column = 0;
    int row = 0;
    int facing = 0;                         // 0 = east
    int step = 0;
    bool atFlag = false;

    while (!atFlag && step < maxSteps) {
        step = step + 1;
        int nextColumn = column + moveColumn[facing];
        int nextRow = row + moveRow[facing];
        bool wallAhead = nextColumn < 0 || nextColumn >= size || nextRow < 0 || nextRow >= size;
        if (wallAhead) {
            facing = (facing + 1) % 4;      // turn right
            std::cout << "step " << step << ": wall ahead -> turn right, now facing "
                      << name[facing] << '\n';
        } else {
            column = nextColumn;
            row = nextRow;
            std::cout << "step " << step << ": forward to column " << column << ", row "
                      << row << '\n';
        }
        atFlag = (column == flagColumn && row == flagRow);
    }
    if (atFlag) {
        std::cout << "reached the flag\n";
    } else {
        std::cout << "safety limit: stopped after " << step << " steps, flag not reached\n";
    }
    return 0;
}
