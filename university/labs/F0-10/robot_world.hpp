// A tiny robot world written for KID101 (chapter F0-10). Our own teaching model:
// a grid with walls (#), a robot (starts at column 1, row 1, facing east) and a goal (G).
// The robot has one sensor (is there a wall in the square ahead?) and two motor
// actions (drive forward one square, turn right a quarter). It is not a model of
// any real robot kit.
#include <iostream>
#include <string>
#include <vector>

struct Robot
{
    const std::vector<std::string> map = {
        "#######",
        "#    ##",
        "#### ##",
        "#G   ##",
        "#######",
    };
    int column = 1;
    int row = 1;
    int facing = 0; // 0 east, 1 south, 2 west, 3 north
    int bumps = 0;

    int aheadColumn() const { return column + (facing == 0 ? 1 : facing == 2 ? -1 : 0); }
    int aheadRow() const { return row + (facing == 1 ? 1 : facing == 3 ? -1 : 0); }

    bool sensorWallAhead() const { return map[aheadRow()][aheadColumn()] == '#'; }
    bool atGoal() const { return map[row][column] == 'G'; }

    void motorForward()
    {
        if (sensorWallAhead()) {
            bumps = bumps + 1;
            std::cout << "    BUMP! the robot drove into a wall and did not move\n";
            return;
        }
        column = aheadColumn();
        row = aheadRow();
    }
    void motorTurnRight() { facing = (facing + 1) % 4; }

    void report(int tick, const std::string& did) const
    {
        const std::vector<std::string> names = {"east", "south", "west", "north"};
        std::cout << "tick " << tick << ": " << did << " -> column " << column << ", row " << row
                  << ", facing " << names[facing] << '\n';
    }
};
