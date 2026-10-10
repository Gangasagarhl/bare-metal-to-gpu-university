// F0-47 number check: recomputes every number used in the chapter text.
#include <iostream>

struct Vec3
{
    double x;
    double y;
    double z;
};

Vec3 add(Vec3 a, Vec3 b)
{
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}

Vec3 scale(double k, Vec3 a)
{
    return {k * a.x, k * a.y, k * a.z};
}

void print(const char* label, Vec3 v)
{
    std::cout << label << " = (" << v.x << ", " << v.y << ", " << v.z << ")\n";
}

int main()
{
    // Worked example: a drone's three legs, metres east, north, up from the take-off pad.
    const Vec3 leg1{4.0, 3.0, 0.0};
    const Vec3 leg2{0.0, 0.0, 2.5};
    const Vec3 leg3{-1.0, 2.0, 0.0};
    const Vec3 end = add(add(leg1, leg2), leg3);
    print("end of the three legs", end);
    print("leg2 + leg1 (order swapped)", add(leg2, leg1));
    print("leg1 + leg2", add(leg1, leg2));
    print("way home = -1 x end", scale(-1.0, end));
    print("half of leg1", scale(0.5, leg1));
    print("2 x (1,0,0) + 3 x (0,1,0)", add(scale(2.0, {1.0, 0.0, 0.0}), scale(3.0, {0.0, 1.0, 0.0})));
    // Layer 2 examples
    print("(2,1,0) + (1,3,0)", add({2.0, 1.0, 0.0}, {1.0, 3.0, 0.0}));
    print("(5,4,0) - (2,1,0)", add({5.0, 4.0, 0.0}, scale(-1.0, {2.0, 1.0, 0.0})));
    print("3 x (2,-1,0)", scale(3.0, {2.0, -1.0, 0.0}));
    // Check yourself numbers
    print("(1,2,0) + (4,-3,0)", add({1.0, 2.0, 0.0}, {4.0, -3.0, 0.0}));
    print("B - A for A=(2,7,0), B=(5,3,0)", add({5.0, 3.0, 0.0}, scale(-1.0, {2.0, 7.0, 0.0})));
    print("-2 x (3,-1,4)", scale(-2.0, {3.0, -1.0, 4.0}));
    print("(1,1,1) + (2,0,-1) + (0,3,0)", add(add({1.0, 1.0, 1.0}, {2.0, 0.0, -1.0}), {0.0, 3.0, 0.0}));
    print("4 x (1,0,0) - 1 x (0,1,0)", add(scale(4.0, {1.0, 0.0, 0.0}), scale(-1.0, {0.0, 1.0, 0.0})));
    return 0;
}
