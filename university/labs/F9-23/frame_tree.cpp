// F9-23 Listing 2: a robot base, a camera and an arm shoulder in one frame tree.
// The camera sees a cup; the tree tells the arm where the cup is.
#include "frames.hpp"
#include <cstdio>

void show(const char* what, const Vec3& p)
{
    std::printf("%-38s (%7.4f, %7.4f, %7.4f)\n", what, p[0], p[1], p[2]);
}

int main()
{
    FrameTree tree("world");
    // where the robot base stands: (2, 1, 0), turned 90 degrees to the left (yaw)
    tree.set("world", "base", {rotZ(rad(90)), {2.0, 1.0, 0.0}});
    // camera bolted 0.2 m ahead of and 0.6 m above the base origin, pitched 30 degrees down
    tree.set("base", "camera", {rotY(rad(30)), {0.2, 0.0, 0.6}});
    // arm shoulder 0.1 m ahead of and 0.3 m above the base origin, not turned
    tree.set("base", "shoulder", {rotZ(0.0), {0.1, 0.0, 0.3}});

    const Vec3 cupInCamera{0.8, 0.0, 0.0}; // 0.8 m straight along the camera's x axis
    show("cup in camera frame:", cupInCamera);
    show("cup in base frame:", applyPoint(tree.lookup("base", "camera"), cupInCamera));
    show("cup in shoulder frame (arm goal):",
         applyPoint(tree.lookup("shoulder", "camera"), cupInCamera));
    show("cup in world frame (map):", applyPoint(tree.lookup("world", "camera"), cupInCamera));

    // a direction: the camera's viewing axis, seen from the world (no shift applied)
    show("camera x axis in world frame:",
         applyDirection(tree.lookup("world", "camera"), {1, 0, 0}));

    // round trip: world -> camera -> world must give the same point back
    const Vec3 w = applyPoint(tree.lookup("world", "camera"), cupInCamera);
    const Vec3 back = applyPoint(tree.lookup("camera", "world"), w);
    show("round trip back in camera frame:", back);

    // the robot drives: only one edge changes, every lookup follows automatically
    tree.set("world", "base", {rotZ(rad(180)), {0.0, 0.0, 0.0}});
    show("after the robot moves, cup in world:",
         applyPoint(tree.lookup("world", "camera"), cupInCamera));
    show("... and in the shoulder frame:",
         applyPoint(tree.lookup("shoulder", "camera"), cupInCamera));
    return 0;
}
