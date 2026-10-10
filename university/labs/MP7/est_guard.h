// est_guard.h - MP7 starter module: an estimator health guard for the DN401 course
// simulator (labs/F10-33/dronesim.hpp). It watches the inputs the navigation trusts and
// raises a latched fault when they stop being believable:
//   MagField  - compass field strength away from its ground value for too long
//               (the F10-38 Log A flyaway: current-dependent compass interference);
//   GnssJump  - GNSS position disagrees with the guard's own prediction for too long;
//   GnssStale - GNSS horizontal position has not changed at all for too long.
// The guard only reports. The integration (mp7_sitl.h) turns a fault into a failsafe.
// Parameter names EG_* belong to this course only; they are NOT PX4 or ArduPilot names.
#pragma once

#include <cmath>
#include <string>
#include <vector>

#include "../F10-33/dronesim.hpp"

namespace mp7 {

enum Fault : unsigned { kNone = 0, kMagField = 1, kGnssJump = 2, kGnssStale = 4 };

inline std::string fault_names(unsigned f)
{
    std::string s;
    if (f & kMagField) s += "MAG_FIELD ";
    if (f & kGnssJump) s += "GNSS_JUMP ";
    if (f & kGnssStale) s += "GNSS_STALE ";
    if (s.empty()) return "none";
    s.pop_back();
    return s;
}

struct GuardParams
{
    bool enable = true;         // EG_ENABLE
    double mag_tol = 0.25;      // EG_MAG_TOL   allowed |field strength - ground value|
    double mag_hold_s = 0.5;    // EG_MAG_HOLD  how long it may stay outside, s
    double pos_gate_m = 2.0;    // EG_POS_GATE  allowed horizontal innovation, m
    double pos_hold_s = 0.3;    // EG_POS_HOLD  how long it may stay outside, s
    double stale_s = 0.5;       // EG_GNSS_STALE  unchanged horizontal position, s
    double filt_tau_s = 0.5;    // EG_FILT_TAU  time constant of the guard's position filter, s

    std::vector<std::string> validate() const  // empty = acceptable
    {
        std::vector<std::string> e;
        if (!(mag_tol > 0 && mag_tol < 1)) e.push_back("EG_MAG_TOL must be in (0, 1)");
        if (!(mag_hold_s >= 0 && mag_hold_s <= 5)) e.push_back("EG_MAG_HOLD must be in [0, 5] s");
        if (!(pos_gate_m > 0)) e.push_back("EG_POS_GATE must be > 0 m");
        if (!(pos_hold_s >= 0 && pos_hold_s <= 5)) e.push_back("EG_POS_HOLD must be in [0, 5] s");
        if (!(stale_s > 0)) e.push_back("EG_GNSS_STALE must be > 0 s");
        if (!(filt_tau_s > 0)) e.push_back("EG_FILT_TAU must be > 0 s");
        return e;
    }
};

struct GuardStatus  // what the module publishes every cycle
{
    double t = 0;
    double mag_dev = 0;    // |field strength - ground value|
    double pos_innov = 0;  // horizontal innovation, m
    double stale_for = 0;  // s since the horizontal GNSS position last changed
    unsigned faults = kNone;  // latched: once set, stays set until disarm
    double first_fault_t = -1;
};

class EstGuard
{
public:
    explicit EstGuard(GuardParams p) : p_(p) {}

    const GuardParams& params() const { return p_; }

    // Called once before arming, on the ground, with the motors stopped.
    void reset_on_ground(const dn::Sensors& s)
    {
        st_ = GuardStatus{};
        mag_ref_ = s.mag_norm;
        est_ = s.gps_pos;
        last_xy_ = s.gps_pos;
        last_t_ = s.t;
        last_change_t_ = s.t;
        mag_out_since_ = pos_out_since_ = -1;
        started_ = true;
    }

    GuardStatus update(const dn::Sensors& s)
    {
        if (!started_) reset_on_ground(s);
        const double dt = s.t - last_t_;  // from the timestamps, never assumed
        last_t_ = s.t;
        st_.t = s.t;
        if (!p_.enable || dt <= 0) return st_;

        // 1. compass field strength against its value on the ground
        st_.mag_dev = std::fabs(s.mag_norm - mag_ref_);
        check_hold(st_.mag_dev > p_.mag_tol, mag_out_since_, p_.mag_hold_s, kMagField, s.t);

        // 2. GNSS position against the guard's own prediction (velocity integrated)
        const dn::V3 pred = est_ + dt * s.gps_vel;
        const dn::V3 innov = s.gps_pos - pred;
        st_.pos_innov = dn::hnorm(innov);
        const double a = 1.0 - std::exp(-dt / p_.filt_tau_s);
        est_ = pred + a * innov;
        check_hold(st_.pos_innov > p_.pos_gate_m, pos_out_since_, p_.pos_hold_s, kGnssJump, s.t);

        // 3. horizontal GNSS position that never changes (a frozen receiver or driver)
        if (s.gps_pos.x != last_xy_.x || s.gps_pos.y != last_xy_.y) last_change_t_ = s.t;
        last_xy_ = s.gps_pos;
        st_.stale_for = s.t - last_change_t_;
        if (st_.stale_for > p_.stale_s) raise(kGnssStale, s.t);
        return st_;
    }

private:
    void check_hold(bool out, double& since, double hold, Fault f, double t)
    {
        if (!out) {
            since = -1;
            return;
        }
        if (since < 0) since = t;
        if (t - since >= hold - 1e-9) raise(f, t);
    }

    void raise(Fault f, double t)
    {
        if (st_.faults == kNone) st_.first_fault_t = t;
        st_.faults |= f;
    }

    GuardParams p_;
    GuardStatus st_;
    dn::V3 est_, last_xy_;
    double mag_ref_ = 1.0, last_t_ = 0, last_change_t_ = 0;
    double mag_out_since_ = -1, pos_out_since_ = -1;
    bool started_ = false;
};

}  // namespace mp7
