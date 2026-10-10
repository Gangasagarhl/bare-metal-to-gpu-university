// cascade.hpp - DN301's cascaded multirotor controller (F10-16), for the DN201 simulator.
//   position P -> velocity PID -> thrust vector -> attitude P (quaternion error) -> rate PID
//   -> torques and thrust -> mixer (F10-02; F10-18 replaces it).
// Axes as in DN201: x forward, y left, z up (world and body). Gains are exercise values for
// the course quad; rates are this course's choices, not those of any autopilot.
#pragma once
#include "../F10-04/quadsim.hpp"

#include <algorithm>
#include <cmath>

namespace dn301 {

// A PID for one axis: P on error, I with clamp, D on the measurement through a low-pass
// filter (no kick on setpoint steps), output clamp. Same structure as RB202's F9-21.
struct Pid
{
    double kp = 0.0, ki = 0.0, kd = 0.0;
    double iLimit = 0.0, outLimit = 1e9; // symmetric limits
    double dTau = 0.0;                   // s, derivative filter time constant (0 = none)
    double integral = 0.0, prev = 0.0, dFilt = 0.0;
    bool started = false;

    double update(double setpoint, double measured, double dt)
    {
        const double e = setpoint - measured;
        integral = std::clamp(integral + ki * e * dt, -iLimit, iLimit);
        double rate = 0.0;
        if (started) {
            rate = (measured - prev) / dt;
        }
        prev = measured;
        started = true;
        dFilt += (dt / (dTau + dt)) * (rate - dFilt);
        return std::clamp(kp * e + integral - kd * dFilt, -outLimit, outLimit);
    }
};

struct Gains
{
    double posP = 1.0;                             // 1/s: position error -> velocity setpoint
    double velP = 2.0, velI = 0.5, velD = 0.0;     // 1/s, 1/s^2, -: velocity error -> acceleration
    double attP = 6.0, yawP = 3.0;                 // 1/s: attitude error -> rate setpoint
    double rateP = 20.0, rateI = 5.0, rateD = 0.3; // roll and pitch: rate error -> angular accel
    double rollRatePScale = 1.0; // roll's rate P = rateP * this (1 = same as pitch)
    double yawRateP = 8.0, yawRateI = 2.0;
    double rateDTau = 0.005; // s, D-term filter
    double maxTilt = 35.0 * std::numbers::pi / 180.0;
    double maxHorizSpeed = 5.0, maxVertSpeed = 2.0; // m/s
    double maxRate = 4.0;                           // rad/s, roll/pitch rate setpoint limit
};

struct Setpoint
{
    dn::Vec3 pos;
    double yaw = 0.0;
};

// What each loop asked for at its last run: logged for setpoint-versus-actual plots.
struct Trace
{
    dn::Vec3 velSp, accSp, rateSp, torque;
    dn::Euler attSp;
    double thrust = 0.0;
};

class Cascade
{
public:
    // Loop periods: rate 2 ms (500 Hz), attitude 4 ms (250 Hz), velocity and position 20 ms.
    static constexpr int ratePeriodMs = 2, attPeriodMs = 4, outerPeriodMs = 20;

    explicit Cascade(const dn::Params& p, const Gains& g = Gains{}) : P_(p), G_(g)
    {
        trace.thrust = P_.mass * P_.g; // hover until the outer loops have run once
        for (Pid* v : {&vel_[0], &vel_[1], &vel_[2]}) {
            *v = Pid{G_.velP, G_.velI, G_.velD, 3.0, 1e9, 0.0};
        }
        for (Pid* r : {&rate_[0], &rate_[1]}) {
            *r = Pid{G_.rateP, G_.rateI, G_.rateD, 50.0, 1e9, G_.rateDTau};
        }
        rate_[0].kp *= G_.rollRatePScale;
        rate_[2] = Pid{G_.yawRateP, G_.yawRateI, 0.0, 20.0, 1e9, 0.0};
    }

    // Call once per millisecond with the state estimate; returns rotor speed commands.
    // Bypass switches let a test drive one inner loop directly.
    dn::Rotors step(const dn::State& est, const Setpoint& sp, int ms)
    {
        if (ms % outerPeriodMs == 0 && !holdOuter) {
            outer(est, sp);
        }
        if (ms % attPeriodMs == 0 && !holdAttitude) {
            attitude(est);
        }
        if (ms % ratePeriodMs == 0) {
            rates(est);
        }
        return command();
    }

    // The rotor commands for the current setpoints, without running any loop.
    dn::Rotors command() const { return allocator(P_, trace.thrust, trace.torque); }

    // The mixer: wanted thrust and torques -> rotor commands. Default: DN201's mixer, which
    // clips each rotor at zero and at full speed on its own (F10-18 replaces it).
    using Allocator = dn::Rotors (*)(const dn::Params&, double, dn::Vec3);
    Allocator allocator = &dn::mix;

    Trace trace;               // the latest setpoints of every loop
    bool holdOuter = false;    // true: velocity/position loops off; attitude and thrust from trace
    bool holdAttitude = false; // true: attitude loop off; rate setpoint from trace
    bool velocityOnly = false; // true: position loop off; velocity setpoint from trace
    Gains& gains() { return G_; }

private:
    void outer(const dn::State& est, const Setpoint& sp)
    {
        const double dt = outerPeriodMs * 1e-3;
        if (!velocityOnly) {
            dn::Vec3 v = G_.posP * (sp.pos - est.p);
            const double h = std::hypot(v.x, v.y);
            if (h > G_.maxHorizSpeed) {
                v.x *= G_.maxHorizSpeed / h;
                v.y *= G_.maxHorizSpeed / h;
            }
            v.z = std::clamp(v.z, -G_.maxVertSpeed, G_.maxVertSpeed);
            trace.velSp = v;
        }
        dn::Vec3 a{vel_[0].update(trace.velSp.x, est.v.x, dt),
                   vel_[1].update(trace.velSp.y, est.v.y, dt),
                   vel_[2].update(trace.velSp.z, est.v.z, dt)};
        // Tilt limit: the horizontal acceleration may not exceed (g + az) tan(maxTilt).
        const double up = std::max(P_.g + a.z, 0.2 * P_.g);
        const double hMax = up * std::tan(G_.maxTilt);
        const double h = std::hypot(a.x, a.y);
        if (h > hMax) {
            a.x *= hMax / h;
            a.y *= hMax / h;
        }
        trace.accSp = a;
        // Thrust vector F = m (a + g z); its direction is the wanted body z axis.
        const dn::Vec3 F = P_.mass * dn::Vec3{a.x, a.y, up};
        const double c = std::cos(sp.yaw), s = std::sin(sp.yaw);
        const dn::Vec3 Fh{c * F.x + s * F.y, -s * F.x + c * F.y, F.z}; // in the heading frame
        const double n = std::sqrt(dn::dot(F, F));
        trace.attSp = {std::asin(std::clamp(-Fh.y / n, -1.0, 1.0)), std::atan2(Fh.x, Fh.z), sp.yaw};
        // Thrust: the part of F along the current body z axis.
        const dn::Vec3 bodyZ = dn::rotate(est.q, dn::Vec3{0.0, 0.0, 1.0});
        trace.thrust = std::max(dn::dot(F, bodyZ), 0.0);
    }

    void attitude(const dn::State& est)
    {
        const dn::Quat qd = dn::fromEuler(trace.attSp);
        const dn::Quat qe = dn::mul(dn::Quat{est.q.w, -est.q.x, -est.q.y, -est.q.z}, qd);
        const double sgn = qe.w >= 0.0 ? 2.0 : -2.0; // shortest way round
        trace.rateSp = {std::clamp(G_.attP * sgn * qe.x, -G_.maxRate, G_.maxRate),
                        std::clamp(G_.attP * sgn * qe.y, -G_.maxRate, G_.maxRate),
                        G_.yawP * sgn * qe.z};
    }

    void rates(const dn::State& est)
    {
        const double dt = ratePeriodMs * 1e-3;
        const dn::Vec3 J = P_.inertia;
        trace.torque = {J.x * rate_[0].update(trace.rateSp.x, est.w.x, dt),
                        J.y * rate_[1].update(trace.rateSp.y, est.w.y, dt),
                        J.z * rate_[2].update(trace.rateSp.z, est.w.z, dt)};
    }

    dn::Params P_;
    Gains G_;
    Pid vel_[3], rate_[3];
};

} // namespace dn301
