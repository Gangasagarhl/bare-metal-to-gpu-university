// F9-23 forensic evidence generator: "The arm misses the cup".
// Simulated robot: the true camera mount is known to the simulator. The robot software's
// frame tree was configured by hand, and this program prints what that software logged.
// (Which edge is wrong, and how, is described only in the chapter's answer key.)
#include "frames.hpp"
#include <cmath>
#include <cstdio>

int main()
{
    // --- the physical robot (simulator ground truth) ---
    const Transform T_BC_true{rotY(rad(30)), {0.2, 0.0, 0.6}}; // base <- camera
    const Transform T_BS_true{rotZ(0.0), {0.1, 0.0, 0.3}};     // base <- shoulder

    // --- the robot software's frame tree, as configured ---
    FrameTree tree("base");
    tree.set("base", "camera", inverse(T_BC_true));
    tree.set("base", "shoulder", {rotZ(0.0), {0.1, 0.0, 0.3}});

    std::printf("== frame tree dump (parent -> child: translation; camera x axis in parent) ==\n");
    for (const char* child : {"camera", "shoulder"}) {
        const Transform T = tree.lookup("base", child);
        const Vec3 ax = applyDirection(T, {1, 0, 0});
        std::printf("base -> %-8s t = (%7.4f, %7.4f, %7.4f)  x axis = (%7.4f, %7.4f, %7.4f)\n",
                    child, T.t[0], T.t[1], T.t[2], ax[0], ax[1], ax[2]);
    }

    std::printf("\n== pick attempts (all coordinates in metres) ==\n");
    std::printf("cup  seen by camera (camera frame)      arm goal (shoulder frame)      "
                "cup found at (shoulder frame)  miss\n");
    const Vec3 cupsInBase[] = {
        {0.7, -0.2, 0.2}, {0.9, 0.0, 0.2}, {1.0, 0.15, 0.2}, {0.8, 0.3, 0.2}};
    int n = 1;
    for (const Vec3& pB : cupsInBase) {
        const Vec3 seen = applyPoint(inverse(T_BC_true), pB); // what the camera reports
        const Vec3 goal = applyPoint(tree.lookup("shoulder", "camera"), seen);
        const Vec3 truth = applyPoint(inverse(T_BS_true), pB); // where the cup really is
        const double miss = std::hypot(goal[0] - truth[0], goal[1] - truth[1], goal[2] - truth[2]);
        std::printf("%d    (%7.4f, %7.4f, %7.4f)       (%7.4f, %7.4f, %7.4f)      "
                    "(%7.4f, %7.4f, %7.4f)      %.3f\n",
                    n++, seen[0], seen[1], seen[2], goal[0], goal[1], goal[2], truth[0], truth[1],
                    truth[2], miss);
    }
    return 0;
}
