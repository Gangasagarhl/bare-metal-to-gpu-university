// F9-29 Listing 3: load the course robot's URDF, print its tree, and verify the arm's
// forward kinematics from the file against the closed-form formula of F9-25.
#include "urdf_mini.hpp"
#include <algorithm>
#include <cstdio>
#include <numbers>

void printTree(const Model& m, const std::string& link, int depth)
{
    for (const auto& [child, j] : m.byChild) {
        if (j.parent != link) continue;
        std::printf("%*s%s --[%s: %s]--> %s\n", 2 * depth, "", link.c_str(), j.type.c_str(),
                    j.name.c_str(), child.c_str());
        printTree(m, child, depth + 1);
    }
}

int main()
{
    const Model m = Model::load("course_robot.urdf");
    std::printf("robot \"%s\", root link %s, %zu links, %zu joints\n", m.name.c_str(),
                m.root.c_str(), m.mass.size(), m.byChild.size());
    printTree(m, m.root, 0);
    double total = 0;
    for (const auto& [link, kg] : m.mass) total += kg;
    std::printf("total mass from <inertial>: %.2f kg\n", total);

    // closed form (F9-25) plus the mount offset (0.1, 0, 0.3) of the arm on the base
    auto closed = [](double q1, double q2, double q3) {
        const double reach = 0.30 * std::cos(q2) + 0.30 * std::cos(q2 + q3);
        return V3{0.1 + reach * std::cos(q1), reach * std::sin(q1),
                  0.3 + 0.10 + 0.30 * std::sin(q2) + 0.30 * std::sin(q2 + q3)};
    };
    const double d = std::numbers::pi / 180;
    const Tf home = m.pose("tool", {});
    std::printf("tool at home pose, base_link frame: (%.4f, %.4f, %.4f)\n", home.t[0], home.t[1],
                home.t[2]);
    double worst = 0;
    int n = 0;
    for (int a = -180; a <= 180; a += 20)
        for (int b = -90; b <= 90; b += 10)
            for (int c = -140; c <= 140; c += 20) {
                const Tf T =
                    m.pose("tool", {{"base_yaw", a * d}, {"shoulder", b * d}, {"elbow", c * d}});
                const V3 g = closed(a * d, b * d, c * d);
                for (int i = 0; i < 3; ++i) worst = std::max(worst, std::abs(T.t[i] - g[i]));
                ++n;
            }
    std::printf("checked %d configurations: largest difference file vs closed form %.2e m\n", n,
                worst);

    // wheels: where are they, and does spinning a wheel move its origin? (it must not)
    for (const char* w : {"left_wheel", "right_wheel"}) {
        const Tf T = m.pose(w, {{std::string(w) + "_joint", 1.0}});
        std::printf("%-11s origin (%.3f, %.3f, %.3f)\n", w, T.t[0], T.t[1], T.t[2]);
    }
    // joint limits read from the file
    for (const char* j : {"base_yaw", "shoulder", "elbow"}) {
        for (const auto& [child, jm] : m.byChild)
            if (jm.name == j) std::printf("limit %-9s [%7.4f, %7.4f] rad\n", j, jm.lower, jm.upper);
    }
    return worst < 1e-12 ? 0 : 1;
}
