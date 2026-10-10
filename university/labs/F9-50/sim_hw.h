// sim_hw.h - F9-50 Listing 2: a simulated two-wheel drive behind the uctl::Hardware interface.
// The "device" is a model: each wheel's speed follows its motor command with a first-order
// lag, and an encoder reports whole counts. All parameters are simulation choices of this
// course, not the data of a real motor or encoder.
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <vector>

#include "uctl.h"

class SimWheels : public uctl::Hardware {
public:
    void init() override
    {
        // per-wheel model parameters: speed per unit command (rad/s) and time constant (s)
        gain_ = {20.0, 18.0};          // the right motor is a little weaker
        tau_ = {0.050, 0.060};
    }
    void activate() override { effortCmd_ = {0.0, 0.0}; }    // start with the motors off

    std::vector<uctl::StateInterface> exportStates() override
    {
        return {{"left_wheel/position", &pos_[0]}, {"left_wheel/velocity", &vel_[0]},
                {"right_wheel/position", &pos_[1]}, {"right_wheel/velocity", &vel_[1]}};
    }
    std::vector<uctl::CommandInterface> exportCommands() override
    {
        return {{"left_wheel/effort", &effortCmd_[0]}, {"right_wheel/effort", &effortCmd_[1]}};
    }

    void read(double dt) override                 // encoder counts -> position and velocity
    {
        for (std::size_t i = 0; i < 2; ++i) {
            const long counts = static_cast<long>(std::floor(trueAngle_[i] / kRadPerCount));
            const double newPos = static_cast<double>(counts) * kRadPerCount;
            const double raw = (newPos - pos_[i]) / dt;          // difference of two readings
            vel_[i] += 0.2 * (raw - vel_[i]);                    // light low-pass filter
            pos_[i] = newPos;
        }
    }

    void write(double dt) override                // command -> the simulated motors move
    {
        for (std::size_t i = 0; i < 2; ++i) {
            const double u = std::clamp(effortCmd_[i], -1.0, 1.0);   // the driver saturates
            trueSpeed_[i] += dt * (gain_[i] * u - trueSpeed_[i]) / tau_[i];
            trueAngle_[i] += dt * trueSpeed_[i];
        }
    }

private:
    static constexpr double kRadPerCount = 2.0 * 3.141592653589793 / 4096.0;
    std::array<double, 2> gain_{}, tau_{};
    std::array<double, 2> effortCmd_{}, pos_{}, vel_{};       // exported interfaces
    std::array<double, 2> trueSpeed_{}, trueAngle_{};         // the "physics", hidden
};
