// F9-23 Listing 1: rigid transforms in 3D and a tiny frame tree.
// Convention (same as MA201 F0-53/F0-54): T_AB maps coordinates in frame B
// to coordinates in frame A. A chain T_AC = T_AB * T_BC needs matching inner letters.
#pragma once
#include <array>
#include <cmath>
#include <map>
#include <numbers>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using Vec3 = std::array<double, 3>;
using Mat3 = std::array<Vec3, 3>; // row-major 3 x 3

inline double rad(double deg)
{
    return deg * std::numbers::pi / 180.0;
}

inline Mat3 rotX(double a)
{
    const double c = std::cos(a), s = std::sin(a);
    return {{{1, 0, 0}, {0, c, -s}, {0, s, c}}};
}
inline Mat3 rotY(double a)
{
    const double c = std::cos(a), s = std::sin(a);
    return {{{c, 0, s}, {0, 1, 0}, {-s, 0, c}}};
}
inline Mat3 rotZ(double a)
{
    const double c = std::cos(a), s = std::sin(a);
    return {{{c, -s, 0}, {s, c, 0}, {0, 0, 1}}};
}

inline Mat3 mul(const Mat3& a, const Mat3& b)
{
    Mat3 c{};
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            for (int k = 0; k < 3; ++k) c[i][j] += a[i][k] * b[k][j];
    return c;
}
inline Vec3 mul(const Mat3& a, const Vec3& v)
{
    Vec3 r{};
    for (int i = 0; i < 3; ++i)
        for (int k = 0; k < 3; ++k) r[i] += a[i][k] * v[k];
    return r;
}
inline Mat3 transpose(const Mat3& a)
{
    Mat3 t{};
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j) t[i][j] = a[j][i];
    return t;
}

struct Transform { // [R t; 0 0 0 1] stored as its two parts
    Mat3 R{{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}};
    Vec3 t{0, 0, 0};
};

inline Transform operator*(const Transform& ab, const Transform& bc) // T_AC = T_AB T_BC
{
    const Vec3 rt = mul(ab.R, bc.t);
    return {mul(ab.R, bc.R), {rt[0] + ab.t[0], rt[1] + ab.t[1], rt[2] + ab.t[2]}};
}
inline Transform inverse(const Transform& ab) // T_BA = [R^T, -R^T t]
{
    const Mat3 rt = transpose(ab.R);
    const Vec3 v = mul(rt, ab.t);
    return {rt, {-v[0], -v[1], -v[2]}};
}
inline Vec3 applyPoint(const Transform& ab, const Vec3& pB) // p_A = R p_B + t
{
    const Vec3 r = mul(ab.R, pB);
    return {r[0] + ab.t[0], r[1] + ab.t[1], r[2] + ab.t[2]};
}
inline Vec3 applyDirection(const Transform& ab, const Vec3& dB)
{
    return mul(ab.R, dB);
}

// A frame tree: every frame except the root stores its parent and T_parent_child.
class FrameTree {
  public:
    explicit FrameTree(std::string root) : root_(std::move(root)) {}

    void set(const std::string& parent, const std::string& child, const Transform& T_parent_child)
    {
        if (child == root_) throw std::runtime_error("the root has no parent");
        edges_[child] = {parent, T_parent_child};
    }

    // T_root_frame: walk from the frame up to the root, multiplying on the left.
    Transform fromRoot(const std::string& frame) const
    {
        Transform T; // identity
        std::string f = frame;
        int steps = 0;
        while (f != root_) {
            const auto it = edges_.find(f);
            if (it == edges_.end()) throw std::runtime_error("frame not connected: " + f);
            T = it->second.T * T;
            f = it->second.parent;
            if (++steps > 100) throw std::runtime_error("cycle in frame tree");
        }
        return T;
    }

    // T_target_source: maps coordinates in 'source' to coordinates in 'target'.
    Transform lookup(const std::string& target, const std::string& source) const
    {
        return inverse(fromRoot(target)) * fromRoot(source);
    }

  private:
    struct Edge {
        std::string parent;
        Transform T;
    };
    std::string root_;
    std::map<std::string, Edge> edges_;
};
