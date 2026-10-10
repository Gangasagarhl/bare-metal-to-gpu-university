// loop_log.cpp - forensic evidence for F10-03 ("The horizon flipped during the loop").
// The vehicle turns at a constant body rate about its y axis (a loop, nose up first:
// in DN201's axes a negative pitch rate lifts the nose). The attitude is integrated as a
// quaternion (F0-55, F10-04) and shown, as a ground station would, as roll, pitch and yaw.
#include "../F10-04/quadsim.hpp"

#include <cmath>
#include <cstdio>
#include <numbers>

int main()
{
    const double deg = 180.0 / std::numbers::pi;
    const double h = 0.001;
    const dn::Vec3 gyro{0.0, -std::numbers::pi / 2.0, 0.0};  // rad/s: a full loop in 4 s
    dn::Quat q;                                               // starts level, nose forward
    std::printf("  t(s)  gyro_x  gyro_y  gyro_z (deg/s) |   roll  pitch    yaw (deg)\n");
    for (int k = 0; k <= 4000; ++k) {
        if (k % 200 == 0) {
            const dn::Euler e = dn::toEuler(q);
            std::printf("%6.2f %7.1f %7.1f %7.1f        | %6.1f %6.1f %6.1f\n", k * h,
                        gyro.x * deg, gyro.y * deg, gyro.z * deg, e.roll * deg, e.pitch * deg,
                        e.yaw * deg);
        }
        const dn::Quat dq = dn::mul(q, dn::Quat{0.0, gyro.x, gyro.y, gyro.z});
        q = dn::normalized({q.w + 0.5 * h * dq.w, q.x + 0.5 * h * dq.x, q.y + 0.5 * h * dq.y,
                            q.z + 0.5 * h * dq.z});
    }
    return 0;
}
