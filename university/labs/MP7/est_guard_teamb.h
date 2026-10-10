// est_guard_teamb.h - Team B's fork of the MP7 estimator guard (forensic lab evidence).
// Team B copied the class from est_guard.h on day 3 and changed it for "their 50 Hz loop".
// Their own unit tests passed and the bench hover was clean. Read the forensic lab first.
#pragma once

#include "est_guard.h"

namespace mp7 {

class EstGuardTeamB
{
public:
    explicit EstGuardTeamB(GuardParams p) : p_(p) {}

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
        const dn::V3 pred = est_ + 0.02 * s.gps_vel;  // one step of our 50 Hz loop
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
