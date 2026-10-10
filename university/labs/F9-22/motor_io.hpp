// F9-22 Listing 1: the boundary between the control program and the kit.
// The control code talks only to MotorIo. SimulatedKit implements it for this build;
// for the real kit you write a KitMotorIo from your kit's own documentation
// (untested on hardware: no kit was available in this build).
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

class MotorIo
{
public:
    virtual ~MotorIo() = default;
    virtual std::uint16_t readEncoder() = 0;  // free-running counter, wraps after 65535
    virtual void setDuty(double duty) = 0;    // -1.0 (full reverse) .. +1.0 (full forward)
    virtual void advance(double seconds) = 0; // simulation only: let time pass
};

// A pretend kit: the pretend motor of HW302 F1-69 behind a PWM driver on a 12 V
// battery, with a friction deadband and a 1024-count-per-revolution encoder.
// Every number here is an exercise value, not a property of any real product.
class SimulatedKit : public MotorIo
{
public:
    double supplyVolts = 12.0;
    double deadbandVolts = 0.5; // friction: below this the motor does not turn
    double countsPerRev = 1024.0;
    bool wiredBackwards = false; // the sign test of the lab looks for this
    double jamAt = -1.0;         // s after the sign test; < 0 means never (watchdog test)

    std::uint16_t readEncoder() override
    {
        const double counts = std::floor(angle_ * countsPerRev / (2.0 * 3.141592653589793));
        const double wrapped = counts - 65536.0 * std::floor(counts / 65536.0);
        return static_cast<std::uint16_t>(wrapped);
    }

    void setDuty(double duty) override { duty_ = std::clamp(duty, -1.0, 1.0); }

    void advance(double seconds) override
    {
        const int steps = static_cast<int>(std::lround(seconds / 0.0001));
        for (int s = 0; s < steps; ++s) {
            double volts = duty_ * supplyVolts * (wiredBackwards ? -1.0 : 1.0);
            const double magnitude = std::max(0.0, std::abs(volts) - deadbandVolts);
            volts = (volts >= 0.0) ? magnitude : -magnitude;
            speed_ += 0.0001 * (kdc_ * volts - speed_) / tau_;
            clock_ += 0.0001;
            if (jamAt >= 0.0 && clock_ >= 0.7 + jamAt) {
                speed_ = 0.0; // something blocks the shaft
            }
            angle_ += 0.0001 * speed_;
        }
    }

    double trueSpeed() const { return speed_; }

private:
    double tau_ = 1e-5 * 2.0 / (0.01 * 0.01 + 2.0 * 1e-6);
    double kdc_ = 0.01 / (0.01 * 0.01 + 2.0 * 1e-6);
    double duty_ = 0.0;
    double speed_ = 0.0;
    double clock_ = 0.0;    // s since the kit was switched on
    double angle_ = 1000.0; // the counter does not start at zero on a real kit either
};

// Speed from two counter readings taken one period apart. Unsigned subtraction
// wraps the same way the counter does, so a wrap between the readings is harmless
// as long as the shaft turns less than 32768 counts in one period.
inline double speedFromCounts(std::uint16_t now, std::uint16_t before, double countsPerRev,
                              double period)
{
    const std::uint16_t forward = static_cast<std::uint16_t>(now - before);
    const double delta = forward < 32768 ? forward : static_cast<double>(forward) - 65536.0;
    return delta * 2.0 * 3.141592653589793 / countsPerRev / period;
}
