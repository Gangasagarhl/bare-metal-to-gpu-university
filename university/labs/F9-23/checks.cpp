// F9-23 checks: recomputes the numbers used in the chapter's worked example and answers.
#include "frames.hpp"
#include <cstdio>

void show(const char* what, const Vec3& p)
{
    std::printf("%-44s (%8.4f, %8.4f, %8.4f)\n", what, p[0], p[1], p[2]);
}

int main()
{
    // worked example: base at (2, 1, 0), yaw 90; camera 0.2 ahead, 0.6 up, pitched 30 down
    const Transform T_WB{rotZ(rad(90)), {2, 1, 0}};
    const Transform T_BC{rotY(rad(30)), {0.2, 0, 0.6}};
    const Transform T_BS{rotZ(0), {0.1, 0, 0.3}};
    show("step 1: R_y(30) (0.8, 0, 0):", mul(T_BC.R, Vec3{0.8, 0, 0}));
    show("step 2: cup in base:", applyPoint(T_BC, {0.8, 0, 0}));
    show("step 3: cup in shoulder (T_SB T_BC p):", applyPoint(inverse(T_BS) * T_BC, {0.8, 0, 0}));
    show("step 4: cup in world:", applyPoint(T_WB * T_BC, {0.8, 0, 0}));
    const Transform bad = inverse(T_BC);
    show("inverted mount: translation part:", bad.t);
    show("inverted mount: camera x axis in base:", applyDirection(bad, {1, 0, 0}));
    // answer 2: point (1, 0, 0) in a frame at (0, 2, 0) with yaw 90, expressed in the parent
    show("answer 2: T{yaw 90, (0,2,0)} (1,0,0):",
         applyPoint({rotZ(rad(90)), {0, 2, 0}}, {1, 0, 0}));
    // answer 3: the same point treated as a direction
    show("answer 3: same, as a direction:", applyDirection({rotZ(rad(90)), {0, 2, 0}}, {1, 0, 0}));
    // answer 5: tf_time error with a 0.05 s stale pose at 0.5 rad/s and 2 m range: 2 * 2 sin(0.0125)
    std::printf("%-44s %.4f m\n",
                "answer 5: chord 2 * 2 * sin(0.025 / 2):", 2 * 2.0 * std::sin(0.0125));
    std::printf("%-44s %.4f m\n",
                "answer 5: at 5 m range, 1 rad/s, 0.1 s stale:", 2 * 5.0 * std::sin(0.05));
    return 0;
}
