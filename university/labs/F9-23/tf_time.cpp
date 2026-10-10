// F9-23 Listing 3: why transforms carry time stamps. The base turns in place at
// 0.5 rad/s; its pose is recorded every 0.1 s. A camera reading taken at t = 0.95 s
// must be converted with the pose at 0.95 s, not with the newest pose.
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <vector>

struct Stamped {
    double t, yaw; // time (s) and heading of base in world (rad); position stays (0, 0)
};

double yawAt(const std::vector<Stamped>& buf, double t) // linear interpolation
{
    for (std::size_t i = 1; i < buf.size(); ++i) {
        if (t <= buf[i].t) {
            const double a = (t - buf[i - 1].t) / (buf[i].t - buf[i - 1].t);
            return buf[i - 1].yaw + a * (buf[i].yaw - buf[i - 1].yaw);
        }
    }
    return buf.back().yaw; // newer than every sample: a real library would refuse
}

int main()
{
    const double rate = 0.5; // rad/s
    std::vector<Stamped> buf;
    for (int k = 0; k <= 10; ++k) buf.push_back({0.1 * k, rate * 0.1 * k});

    const double tMeas = 0.95; // when the camera saw the object
    const double range = 2.0;  // object 2 m straight ahead of the base at that moment
    const double trueYaw = rate * tMeas;
    const double tx = range * std::cos(trueYaw), ty = range * std::sin(trueYaw);

    const double yawLatest = buf.back().yaw;
    const double yawInterp = yawAt(buf, tMeas);
    const double lx = range * std::cos(yawLatest), ly = range * std::sin(yawLatest);
    const double ix = range * std::cos(yawInterp), iy = range * std::sin(yawInterp);

    std::printf("true object position     (%.4f, %.4f)\n", tx, ty);
    std::printf("newest pose (t = %.2f s)  (%.4f, %.4f)  error %.4f m\n", buf.back().t, lx, ly,
                std::hypot(lx - tx, ly - ty));
    std::printf("pose at t = %.2f s        (%.4f, %.4f)  error %.4f m\n", tMeas, ix, iy,
                std::hypot(ix - tx, iy - ty));
    return 0;
}
