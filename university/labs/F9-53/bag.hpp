// bag.hpp - F9-53 forensic lab: a recorded run ("bag") of odometry and LiDAR messages,
// produced by our simulator, and a mapper that places each scan at the odometry pose
// interpolated at the scan's time stamp. Message format is this lab's own, not ROS's.
#pragma once
#include "slam.hpp"
#include <vector>

namespace rb {

struct OdomMsg { double recv, stamp; Pose pose; };
struct ScanMsg { double recv, stamp, trueTime; std::vector<double> z; };
struct Bag { std::vector<OdomMsg> odom; std::vector<ScanMsg> scans; };

// The route driven at 0.4 m/s, turning on the spot at 1.0 rad/s at every waypoint.
class Timeline
{
public:
    Timeline()
    {
        const std::vector<Pose> wp = routeWaypoints();
        double th = std::atan2(wp[1].y - wp[0].y, wp[1].x - wp[0].x);
        double t = 0.0;
        for (std::size_t k = 0; k + 1 < wp.size(); ++k) {
            const double target = std::atan2(wp[k + 1].y - wp[k].y, wp[k + 1].x - wp[k].x);
            const double turn = wrapAngle(target - th);
            segs_.push_back({t, std::fabs(turn) / 1.0, {wp[k].x, wp[k].y, th}, 0.0, turn});
            t += std::fabs(turn) / 1.0;
            th = target;
            const double len = std::hypot(wp[k + 1].x - wp[k].x, wp[k + 1].y - wp[k].y);
            segs_.push_back({t, len / 0.4, {wp[k].x, wp[k].y, th}, len, 0.0});
            t += len / 0.4;
        }
        end_ = t;
    }
    double end() const { return end_; }
    Pose at(double t) const
    {
        for (const Seg& s : segs_) {
            if (t <= s.t0 + s.dur || &s == &segs_.back()) {
                const double f = s.dur > 0 ? std::clamp((t - s.t0) / s.dur, 0.0, 1.0) : 1.0;
                return compose(s.start, s.len * f, 0.0, s.turn * f);
            }
        }
        return segs_.back().start;
    }
private:
    struct Seg { double t0, dur; Pose start; double len, turn; };
    std::vector<Seg> segs_;
    double end_ = 0.0;
};

// Record the bag. stampOffset is added to every scan's time stamp (0 = correct stamps).
inline Bag record(const Grid& world, const LidarSpec& lidar, double stampOffset)
{
    const Timeline tl;
    Rng rng(5300);
    Bag bag;
    for (int k = 0; 0.02 * k <= tl.end(); ++k) {          // odometry at 50 Hz
        const double t = 0.02 * k;
        bag.odom.push_back({t + 0.002, t, tl.at(t)});
    }
    for (int k = 0; 0.1 * k <= tl.end(); ++k) {           // LiDAR at 10 Hz
        const double t = 0.1 * k;
        bag.scans.push_back({t + 0.16, t + stampOffset, t, scan(world, tl.at(t), lidar, rng)});
    }
    return bag;
}

// Odometry pose at time t, linearly interpolated between the two nearest messages.
inline Pose odomAt(const Bag& bag, double t)
{
    const std::vector<OdomMsg>& o = bag.odom;
    if (t <= o.front().stamp) { return o.front().pose; }
    if (t >= o.back().stamp) { return o.back().pose; }
    std::size_t k = static_cast<std::size_t>((t - o.front().stamp) / 0.02);
    if (k + 1 >= o.size()) { k = o.size() - 2; }
    const double f = (t - o[k].stamp) / (o[k + 1].stamp - o[k].stamp);
    const Pose& a = o[k].pose;
    const Pose& b = o[k + 1].pose;
    return {a.x + f * (b.x - a.x), a.y + f * (b.y - a.y), wrapAngle(a.th + f * wrapAngle(b.th - a.th))};
}

// Turn rate from odometry alone (rad/s), over +-0.05 s around t.
inline double turnRate(const Bag& bag, double t)
{
    return wrapAngle(odomAt(bag, t + 0.05).th - odomAt(bag, t - 0.05).th) / 0.1;
}

// Build a map from the bag; `correction` is subtracted from every scan stamp.
// straightOnly: skip scans whose odometry turn rate is above 0.1 rad/s.
inline OccGrid mapFromBag(const Bag& bag, double maxRange, double correction, bool straightOnly)
{
    OccGrid m;
    for (const ScanMsg& s : bag.scans) {
        if (straightOnly && std::fabs(turnRate(bag, s.stamp - correction)) > 0.1) { continue; }
        m.integrate(odomAt(bag, s.stamp - correction), s.z, maxRange);
    }
    return m;
}

}  // namespace rb
