#include "robot_world.hpp"

int main()
{
    Robot robot;
    const bool wall = robot.sensorWallAhead();
    for (int tick = 1; tick <= 12 && !robot.atGoal(); ++tick) {
        if (wall) {
            robot.motorTurnRight();
            robot.report(tick, "wall ahead, turn right");
        } else {
            robot.motorForward();
            robot.report(tick, "path clear, forward");
        }
    }
    std::cout << (robot.atGoal() ? "goal reached" : "goal not reached") << ", bumps: "
              << robot.bumps << '\n';
    return 0;
}
