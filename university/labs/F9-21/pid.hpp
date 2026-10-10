// pid.hpp - the RB202 PID library (F9-21). Header-only C++20.
// No heap allocation and no exceptions, so the same file can later be used in
// firmware (F9 and F10 courses). One object controls one loop.
//
//   u = P + I + D, then clamped to [outMin, outMax]
//   P = kp * e                     with e = setpoint - measured
//   I = I + ki * period * e      (kept in output units; frozen while it would deepen saturation)
//   D = -kd * filtered d(measured)/dt   (derivative of the measurement: no kick on setpoint steps)
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <optional>

namespace rb {

struct PidConfig
{
    double kp = 0.0;      // output units per error unit
    double ki = 0.0;      // output units per (error unit * second)
    double kd = 0.0;      // output units per (error unit / second)
    double period = 0.01; // s, the fixed time between two update() calls
    double outMin = -1.0; // output limits (for example the driver's voltage range)
    double outMax = 1.0;
    double derivativeTau = 0.0; // s, low-pass filter on the derivative; 0 = no filter
    bool antiWindup = true;     // false only to demonstrate windup
};

inline bool isValid(const PidConfig& c)
{
    const std::array<double, 7> values = {c.kp,     c.ki,     c.kd,           c.period,
                                          c.outMin, c.outMax, c.derivativeTau};
    for (double v : values) {
        if (!std::isfinite(v)) {
            return false;
        }
    }
    return c.kp >= 0.0 && c.ki >= 0.0 && c.kd >= 0.0 && c.period > 0.0 && c.outMin < c.outMax &&
           c.derivativeTau >= 0.0;
}

class Pid
{
public:
    // The only way to make a Pid: an invalid configuration gives no object at all.
    static std::optional<Pid> create(const PidConfig& config)
    {
        if (!isValid(config)) {
            return std::nullopt;
        }
        return Pid(config);
    }

    // One control step; call it exactly once per period.
    double update(double setpoint, double measured)
    {
        if (!std::isfinite(setpoint) || !std::isfinite(measured)) {
            ++rejected_; // keep the state clean; the caller decides what to do
            return output_;
        }
        const double error = setpoint - measured;
        const double p = config_.kp * error;

        double rate = 0.0; // no previous sample yet: no derivative on the first call
        if (hasPrevious_) {
            rate = (measured - previous_) / config_.period;
        }
        previous_ = measured;
        hasPrevious_ = true;
        const double alpha = config_.period / (config_.derivativeTau + config_.period);
        rateFiltered_ += alpha * (rate - rateFiltered_);
        const double d = -config_.kd * rateFiltered_;

        const double candidate = integral_ + config_.ki * config_.period * error;
        const double unclamped = p + candidate + d;
        const bool pushesHigh = unclamped > config_.outMax && error > 0.0;
        const bool pushesLow = unclamped < config_.outMin && error < 0.0;
        if (!config_.antiWindup || !(pushesHigh || pushesLow)) {
            integral_ = candidate; // integrate only when it does not deepen saturation
        }
        if (config_.antiWindup) {
            integral_ = std::clamp(integral_, config_.outMin, config_.outMax);
        }
        output_ = std::clamp(p + integral_ + d, config_.outMin, config_.outMax);
        return output_;
    }

    // Forget the past (for example after an emergency stop), starting from a known output.
    void reset(double output = 0.0)
    {
        integral_ = std::clamp(output, config_.outMin, config_.outMax);
        output_ = integral_;
        rateFiltered_ = 0.0;
        hasPrevious_ = false;
    }

    // Change gains while running. Because the integral is stored in output units,
    // a new ki does not make the output jump.
    bool setGains(double kp, double ki, double kd)
    {
        PidConfig next = config_;
        next.kp = kp;
        next.ki = ki;
        next.kd = kd;
        if (!isValid(next)) {
            return false;
        }
        config_ = next;
        return true;
    }

    double integralTerm() const { return integral_; }
    double lastOutput() const { return output_; }
    long rejectedInputs() const { return rejected_; }
    const PidConfig& config() const { return config_; }

private:
    explicit Pid(const PidConfig& config) : config_(config) {}

    PidConfig config_;
    double integral_ = 0.0;
    double previous_ = 0.0;
    double rateFiltered_ = 0.0;
    double output_ = 0.0;
    bool hasPrevious_ = false;
    long rejected_ = 0;
};

} // namespace rb
