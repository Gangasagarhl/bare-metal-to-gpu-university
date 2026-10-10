// mp6_ekf.hpp - MP6 starter lab (M1): the localisation node. The filter is the F9-34 EKF
// (known landmarks, range-bearing updates) with the F9-35 idea of predicting from encoder
// speed and gyro rate, plus an innovation gate and per-landmark statistics for forensics.
#pragma once
#include "../F9-34/mat.hpp"   // RB302's small matrix type
#include "mp6_robot.hpp"
#include <vector>

namespace mp6 {

struct EkfParams
{
    double sv = 0.05;          // process noise of the speed input, m/s per sqrt(s)
    double sw = 0.03;          // process noise of the turn-rate input, rad/s per sqrt(s)
    double rangeSd = 0.03;     // what the filter believes about the beacon (m)
    double bearingSd = 1.0 * kPi / 180.0;
    double nisGate = 9.21;     // reject a reading whose NIS exceeds this; 0 = no gate
    bool bearingSignBug = false;   // mutant: innovation of the bearing taken with the wrong sign
};

struct LandmarkStats { int seen = 0; int rejected = 0; double sumNis = 0.0; double maxNis = 0.0; };

class Ekf
{
public:
    Ekf(const EkfParams& p, Pose start, int landmarkCount)
        : p_(p), s_(3, 1, {start.x, start.y, start.th}), P_(3, 3, {0.01, 0, 0, 0, 0.01, 0, 0, 0, 0.004}),
          stats_(static_cast<std::size_t>(landmarkCount))
    {
    }

    // Predict with the measured speed v (encoders) and turn rate w (gyro) over dt seconds.
    void predict(double v, double w, double dt)
    {
        const double th = s_(2, 0);
        const Mat G(3, 3, {1.0, 0.0, -v * dt * std::sin(th),
                           0.0, 1.0, v * dt * std::cos(th),
                           0.0, 0.0, 1.0});
        const Mat V(3, 2, {dt * std::cos(th), 0.0, dt * std::sin(th), 0.0, 0.0, dt});
        const Mat M(2, 2, {p_.sv * p_.sv / dt, 0.0, 0.0, p_.sw * p_.sw / dt});   // white noise, density form
        s_ = Mat(3, 1, {s_(0, 0) + v * dt * std::cos(th), s_(1, 0) + v * dt * std::sin(th), wrapAngle(th + w * dt)});
        P_ = G * P_ * G.t() + V * M * V.t();
    }

    // One range-bearing reading of a landmark whose map position is (lx, ly).
    // Returns the NIS; the reading is used only if it passes the gate.
    double update(const BeaconReading& z, double lx, double ly)
    {
        const double dx = lx - s_(0, 0);
        const double dy = ly - s_(1, 0);
        const double q = dx * dx + dy * dy;
        const double r = std::sqrt(q);
        const Mat H(2, 3, {-dx / r, -dy / r, 0.0, dy / q, -dx / q, -1.0});
        Mat y(2, 1, {z.range - r, wrapAngle(z.bearing - (std::atan2(dy, dx) - s_(2, 0)))});
        if (p_.bearingSignBug) { y(1, 0) = -y(1, 0); }
        const Mat R(2, 2, {p_.rangeSd * p_.rangeSd, 0.0, 0.0, p_.bearingSd * p_.bearingSd});
        const Mat S = H * P_ * H.t() + R;
        const Mat Si = inverse(S);
        const double nis = (y.t() * Si * y)(0, 0);
        LandmarkStats& st = stats_[static_cast<std::size_t>(z.id)];
        ++st.seen;
        st.sumNis += nis;
        st.maxNis = std::fmax(st.maxNis, nis);
        if (p_.nisGate > 0.0 && nis > p_.nisGate) {
            ++st.rejected;
            return nis;
        }
        const Mat K = P_ * H.t() * Si;
        s_ = s_ + K * y;
        s_(2, 0) = wrapAngle(s_(2, 0));
        const Mat IKH = Mat::identity(3) - K * H;
        P_ = IKH * P_ * IKH.t() + K * R * K.t();      // Joseph form, as in F9-34
        return nis;
    }

    Pose pose() const { return {s_(0, 0), s_(1, 0), s_(2, 0)}; }
    const Mat& cov() const { return P_; }
    const std::vector<LandmarkStats>& stats() const { return stats_; }

    // Normalised estimation error squared against a ground-truth pose (simulation only).
    double nees(const Pose& truth) const
    {
        const Mat e(3, 1, {s_(0, 0) - truth.x, s_(1, 0) - truth.y, wrapAngle(s_(2, 0) - truth.th)});
        return (e.t() * inverse(P_) * e)(0, 0);
    }

private:
    EkfParams p_;
    Mat s_;
    Mat P_;
    std::vector<LandmarkStats> stats_;
};

}  // namespace mp6
