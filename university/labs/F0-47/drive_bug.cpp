// F0-47 forensic evidence: the robot was told to drive to the fridge.
// This program contains ONE deliberate mistake (see the chapter's forensic lab).
#include <iomanip>
#include <iostream>

struct Vec2
{
    double x;
    double y;
};

Vec2 add(Vec2 a, Vec2 b)
{
    return {a.x + b.x, a.y + b.y};
}

Vec2 subtract(Vec2 a, Vec2 b)
{
    return {a.x - b.x, a.y - b.y};
}

Vec2 scale(double k, Vec2 a)
{
    return {k * a.x, k * a.y};
}

int main()
{
    Vec2 robot{3.0, 5.0};
    const Vec2 fridge{6.0, 5.0};
    std::cout << "target fridge at (" << fridge.x << ", " << fridge.y << ")\n";
    std::cout << "step   robot.x   robot.y   arrow.x   arrow.y\n" << std::fixed << std::setprecision(2);
    for (int step = 0; step <= 3; ++step) {
        const Vec2 arrow = subtract(robot, fridge);
        std::cout << std::setw(4) << step << std::setw(10) << robot.x << std::setw(10) << robot.y
                  << std::setw(10) << arrow.x << std::setw(10) << arrow.y << "\n";
        robot = add(robot, scale(1.0 / 3.0, arrow));
    }
    return 0;
}
