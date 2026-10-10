// mp6_robot.hpp - MP6 starter lab (milestone M1): the simulated robot and its simulated drivers.
// The world is RB401's simulated house (labs/F9-52/house.hpp); planning pieces come from F9-54 and
// F9-56. The robot is a differential-drive disc. Every number here is a choice of this simulator,
// not data of any real kit, motor, encoder, gyro or beacon.
#pragma once
#include "../F9-56/nav.hpp"   // house, grid, Rng, A* planner, costmap, pure pursuit (RB401 labs)
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace mp6 {

using rb::kPi;
using rb::Pose;
using rb::Rng;
using rb::wrapAngle;

struct Landmark { int id; double x, y; };   // a reflector post, position in metres

// Physical truth of the simulated robot (the "kit"); the software has its own copy of
// the calibration values, which may differ (that is how calibration faults are injected).
struct RobotParams
{
    double wheelRadius = 0.050;               // m
    double track = 0.300;                     // m between the wheels' contact points
    int cpr = 1024;                           // encoder counts per wheel revolution
    double maxAccel = 0.8;                    // m/s^2 at each wheel rim (the drive's ramp)
    double slipSd = 0.01;                     // relative noise on each wheel's ground travel
    double gyroBias = 0.004;                  // rad/s, constant, not modelled by the filter
    double gyroSd = 0.01;                     // rad/s per sample
    double rangeSd = 0.03;                    // m, beacon range noise
    double bearingSd = 1.0 * kPi / 180.0;     // rad, beacon bearing noise
    double beaconRange = 4.0;                 // m, beyond this a post is not seen
};

struct EncoderSample { long left, right; };          // cumulative counts
struct BeaconReading { int id; double range, bearing; };

class SimRobot
{
public:
    SimRobot(const rb::Grid& world, const RobotParams& p, const std::vector<Landmark>& posts, Pose start,
             std::uint64_t seed)
        : world_(world), p_(p), posts_(posts), pose_(start), rng_(seed)
    {
    }

    // Motor driver input: wheel angular speed targets in rad/s.
    void setWheelTargets(double omegaL, double omegaR) { targetL_ = omegaL; targetR_ = omegaR; }

    // Advance the physics by dt seconds. A bumper stops the robot on contact with the world.
    void step(double dt)
    {
        const double dmax = p_.maxAccel / p_.wheelRadius * dt;      // ramp limit per step
        omegaL_ += std::clamp(targetL_ - omegaL_, -dmax, dmax);
        omegaR_ += std::clamp(targetR_ - omegaR_, -dmax, dmax);
        angleL_ += omegaL_ * dt;                                     // what the encoders see
        angleR_ += omegaR_ * dt;
        const double dl = omegaL_ * p_.wheelRadius * dt * (1.0 + rng_.gauss(p_.slipSd));   // ground travel
        const double dr = omegaR_ * p_.wheelRadius * dt * (1.0 + rng_.gauss(p_.slipSd));
        const double ds = 0.5 * (dl + dr);
        const double dth = (dr - dl) / p_.track;
        const Pose next{pose_.x + ds * std::cos(pose_.th + 0.5 * dth), pose_.y + ds * std::sin(pose_.th + 0.5 * dth),
                        wrapAngle(pose_.th + dth)};
        if (rb::touches(world_, next) && !rb::touches(world_, pose_)) {
            if (!contact_) { ++collisions_; }
            contact_ = true;
            omegaL_ = 0.0;
            omegaR_ = 0.0;
            speed_ = 0.0;
            return;
        }
        contact_ = false;
        speed_ = ds / dt;
        travelled_ += std::fabs(ds);
        pose_ = next;
        gyroTruth_ = dth / dt;
    }

    EncoderSample encoders() const
    {
        const double k = p_.cpr / (2.0 * kPi);
        return {static_cast<long>(std::floor(angleL_ * k)), static_cast<long>(std::floor(angleR_ * k))};
    }

    double gyro() { return gyroTruth_ + p_.gyroBias + rng_.gauss(p_.gyroSd); }

    // Every post within range and in line of sight gives one range-bearing reading.
    std::vector<BeaconReading> beacons()
    {
        std::vector<BeaconReading> out;
        for (const Landmark& l : posts_) {
            const double dx = l.x - pose_.x;
            const double dy = l.y - pose_.y;
            const double d = std::hypot(dx, dy);
            if (d > p_.beaconRange) { continue; }
            const double a = std::atan2(dy, dx);
            if (rb::castRay(world_, pose_.x, pose_.y, a, d) < d - 0.02) { continue; }   // a wall is in the way
            out.push_back({l.id, d + rng_.gauss(p_.rangeSd), wrapAngle(a - pose_.th + rng_.gauss(p_.bearingSd))});
        }
        return out;
    }

    const Pose& truth() const { return pose_; }
    double speed() const { return speed_; }
    double rimSpeedMax() const { return std::fmax(std::fabs(omegaL_), std::fabs(omegaR_)) * p_.wheelRadius; }
    double travelled() const { return travelled_; }
    int collisions() const { return collisions_; }

private:
    const rb::Grid& world_;
    RobotParams p_;
    std::vector<Landmark> posts_;
    Pose pose_;
    Rng rng_;
    double targetL_ = 0.0, targetR_ = 0.0;
    double omegaL_ = 0.0, omegaR_ = 0.0;
    double angleL_ = 0.0, angleR_ = 0.0;
    double speed_ = 0.0, gyroTruth_ = 0.0, travelled_ = 0.0;
    bool contact_ = false;
    int collisions_ = 0;
};

}  // namespace mp6
