// allocation.cpp - what the course quad really produces when a request does not fit:
// naive clipping versus two priority orders (allocation.hpp). Forces in N, torques in N m;
// each rotor can give 0 to 10 N (kT * maxSpeed^2 of the course quad).
#include "allocation.hpp"

#include <cstdio>

namespace {

void show(const char* label, const alloc::Forces& f, const alloc::Wrench& got)
{
    std::printf("  %-14s f = %5.2f %5.2f %5.2f %5.2f | T %6.2f tx %6.3f ty %6.3f tz %6.4f\n", label,
                f[0], f[1], f[2], f[3], got.T, got.tx, got.ty, got.tz);
}

} // namespace

int main()
{
    const dn::Params P;
    std::printf("rotor force limits: 0 to %.1f N; hover thrust %.2f N\n\n", alloc::maxForce(P),
                P.mass * P.g);
    struct Case
    {
        const char* name;
        alloc::Wrench w;
    };
    const Case cases[] = {
        {"A hover, small roll (fits)", {9.81, 0.10, 0.0, 0.0}},
        {"B hover, hard yaw", {9.81, 0.0, 0.0, 0.20}},
        {"C hover, roll + hard yaw", {9.81, 0.50, 0.0, 0.20}},
        {"D punch-out (38 N) + roll", {38.0, 0.60, 0.0, 0.0}},
        {"E low throttle (2 N) + roll", {2.0, 0.40, 0.0, 0.0}},
        {"F large roll at hover", {9.81, 2.00, 0.0, 0.0}},
        {"G roll beyond authority", {9.81, 4.00, 0.0, 0.0}},
    };
    for (const Case& k : cases) {
        std::printf("%s: wanted T %.2f tx %.3f ty %.3f tz %.4f\n", k.name, k.w.T, k.w.tx, k.w.ty,
                    k.w.tz);
        const alloc::Forces a = alloc::naive(P, k.w);
        show("naive clip", a, alloc::forward(P, a));
        const alloc::Forces b = alloc::prioritised(P, k.w, alloc::Priority::thrustBeforeYaw);
        show("RP > T > Y", b, alloc::forward(P, b));
        const alloc::Forces c = alloc::prioritised(P, k.w, alloc::Priority::yawBeforeThrust);
        show("RP > Y > T", c, alloc::forward(P, c));
    }
    return 0;
}
