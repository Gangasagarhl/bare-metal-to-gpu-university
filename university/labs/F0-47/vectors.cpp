// F0-47 Listing 1: vectors as arrows with numbers (a toy robot on the kitchen floor).
#include <iostream>
#include <vector>

struct Vec2
{
    double x;  // tiles towards the window (east)
    double y;  // tiles away from the door (north)
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

void print(const char* label, Vec2 v)
{
    std::cout << label << " = (" << v.x << ", " << v.y << ")\n";
}

int main()
{
    const Vec2 start{1.0, 2.0};
    const std::vector<Vec2> moves{{3.0, 0.0}, {0.0, 2.0}, {-1.0, 1.0}};

    Vec2 position = start;
    for (const Vec2& m : moves) {
        position = add(position, m);
        print("position after a move", position);
    }

    Vec2 total{0.0, 0.0};
    for (const Vec2& m : moves) {
        total = add(total, m);
    }
    print("sum of the three moves", total);
    print("start + sum", add(start, total));

    const Vec2 fridge{6.0, 5.0};
    print("arrow from robot to fridge (fridge - robot)", subtract(fridge, position));
    print("half of the first move", scale(0.5, moves[0]));
    print("third move reversed", scale(-1.0, moves[2]));
    std::cout << "bytes used by one Vec2 on this machine: " << sizeof(Vec2) << "\n";
    return 0;
}
