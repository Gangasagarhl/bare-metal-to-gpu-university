// controllers.h - F9-50 Listing 3: a wheel-velocity PI controller for the uctl framework.
// One controller drives both wheels: it claims "<wheel>/velocity" states and "<wheel>/effort"
// commands. Targets may be set from another thread (std::atomic, no lock in the loop).
#pragma once
#include <algorithm>
#include <array>
#include <atomic>
#include <string>
#include <vector>

#include "uctl.h"

class WheelVelocityPi : public uctl::Controller {
public:
    std::vector<std::string> wantedStates() const override
    {
        return {"left_wheel/velocity", "right_wheel/velocity"};
    }
    std::vector<std::string> wantedCommands() const override
    {
        return {"left_wheel/effort", "right_wheel/effort"};
    }
    void configure(std::vector<const double*> s, std::vector<double*> c) override
    {
        for (std::size_t i = 0; i < 2; ++i) { vel_[i] = s[i]; effort_[i] = c[i]; }
    }
    void setTarget(std::size_t wheel, double radPerSec) { target_[wheel].store(radPerSec); }

    void update(double /*t*/, double dt) override
    {
        for (std::size_t i = 0; i < 2; ++i) {
            const double error = target_[i].load() - *vel_[i];
            const double unclamped = kp_ * error + ki_ * (integral_[i] + error * dt);
            const double u = std::clamp(unclamped, -1.0, 1.0);
            if (u == unclamped) { integral_[i] += error * dt; }   // anti-windup: no growth
            *effort_[i] = u;                                       // while saturated
        }
    }

private:
    double kp_ = 0.02, ki_ = 1.0;
    std::array<std::atomic<double>, 2> target_{};
    std::array<double, 2> integral_{};
    std::array<const double*, 2> vel_{};
    std::array<double*, 2> effort_{};
};
