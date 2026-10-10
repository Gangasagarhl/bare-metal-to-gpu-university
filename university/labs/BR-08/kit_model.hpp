// BR-08 Listing 2: what changes. A stand-in for the supervised kit.
// The build container has no motor kit, so this model plays the kit. It is our own
// teaching code, NOT a model of any product: every number below is an invented exercise
// value. Each effect of the bridge card is one field, so it can be switched on alone:
//   friction and different parameters, vibration (a once-per-revolution torque),
//   battery sag, an unmodelled driver current limit, a command delay, timing jitter,
//   encoder noise, and a calibration error (the encoder fitted is not the one the
//   software's parameter describes).
// On a real kit, delete this file: the real hardware provides all of it, unasked.
#pragma once
#include "bench.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <deque>
#include <string>

namespace br08 {

struct KitParams
{
    // motor and load (F9-22's model: J 1e-5, R 2, K 0.01, b 1e-6, 0.5 V deadband)
    double inertia = 1.4e-5;     // kg m^2: wheel and gearbox heavier than modelled
    double resistance = 2.2;     // ohm
    double torqueConst = 0.0097; // N m per A (= V s per rad)
    double viscous = 3e-6;       // N m s per rad
    double coulomb = 0.0035;     // N m of dry friction
    double ripple = 0.002;       // N m once per revolution (an eccentric wheel)
    // power
    double batteryOpen = 11.6;   // V, open-circuit voltage at this charge
    double batteryRes = 0.45;    // ohm, internal resistance: sag = current x resistance
    double otherLoad = 1.0;      // A drawn from the same battery by the rest of the robot
    double currentLimit = 2.5;   // A, the driver limits motor current; 0 = no limit
    double commandTimeout = 0.0; // s, driver sets duty 0 if no new command came; 0 = never
    // timing
    double commandDelay = 0.012; // s from setDuty() to the driver applying it
    double latenessMax = 0.0005; // s, each sample is 0..latenessMax late (uniform)
    int burstEvery = 0;          // every N-th sample is extra late (0 = never)
    double burstLate = 0.0;      // s, how late
    // sensors
    double cprTrue = 1000.0;     // counts per revolution of the encoder actually fitted
    bool encoderSwapped = false; // encoder channels A and B exchanged
    double countNoise = 0.3;     // counts, standard deviation of edge-timing noise per read
    double referenceNoise = 0.5; // rad/s, standard deviation of the bench tachometer
    double adcStep = 0.02;       // V, resolution of the supply measurement
    std::uint64_t seed = 8;
};

// Deterministic noise (xorshift64* and Box-Muller): same seed, same run, any machine.
class Rng
{
public:
    explicit Rng(std::uint64_t seed) : s_(seed ? seed : 1) {}
    double uniform()
    {
        s_ ^= s_ >> 12;
        s_ ^= s_ << 25;
        s_ ^= s_ >> 27;
        const std::uint64_t r = (s_ * 2685821657736338717ULL) >> 11;
        return static_cast<double>(r) * (1.0 / 9007199254740992.0);
    }
    double gauss()
    {
        const double u1 = std::max(uniform(), 1e-12);
        const double u2 = uniform();
        return std::sqrt(-2.0 * std::log(u1)) * std::cos(2.0 * 3.141592653589793 * u2);
    }

private:
    std::uint64_t s_;
};

class KitWorld : public LoggedMotorIo
{
public:
    explicit KitWorld(const KitParams& p, std::string label = "kit (stand-in model, kit_model.hpp)")
        : p_(p), rng_(p.seed), refRng_(p.seed * 7919 + 1), label_(std::move(label)),
          volts_(p.batteryOpen - p.batteryRes * p.otherLoad)
    {
    }

    std::uint16_t readEncoder() override
    {
        stamp_ = clock_;
        voltsAtStamp_ = std::round(volts_ / p_.adcStep) * p_.adcStep;
        referenceAtStamp_ = omega_ + p_.referenceNoise * refRng_.gauss();
        const double turns = angle_ / (2.0 * kPi);
        double counts = std::floor(turns * p_.cprTrue + p_.countNoise * rng_.gauss());
        if (p_.encoderSwapped) {
            counts = -counts;
        }
        return static_cast<std::uint16_t>(counts - 65536.0 * std::floor(counts / 65536.0));
    }

    void setDuty(double duty) override
    {
        lastCommand_ = clock_;
        pending_.push_back({clock_ + p_.commandDelay, std::clamp(duty, -1.0, 1.0)});
    }

    // The loop asks for one period; the kit's clock decides when the next sample happens.
    void advance(double seconds) override
    {
        nominal_ += seconds;
        ++samples_;
        double late = p_.latenessMax * rng_.uniform();
        if (p_.burstEvery > 0 && samples_ % p_.burstEvery == 0) {
            late += p_.burstLate;
        }
        const double target = nominal_ + late;
        while (clock_ < target - 1e-9) {
            substep();
        }
    }

    // The hardware emergency stop: opens the motor power path. Software cannot undo it.
    void emergencyStop() { powerCut_ = true; }

    double sampleTime() const override { return stamp_; }
    double supplyVolts() const override { return voltsAtStamp_; }
    double referenceSpeed() const override { return referenceAtStamp_; }
    std::string world() const override { return label_; }
    double trueSpeed() const { return omega_; }
    double motorCurrent() const { return current_; }

private:
    static constexpr double kPi = 3.141592653589793;
    static constexpr double kDt = 1e-4; // s, integration sub-step

    void substep()
    {
        while (!pending_.empty() && pending_.front().first <= clock_ + 1e-12) {
            duty_ = pending_.front().second;
            pending_.pop_front();
        }
        if (p_.commandTimeout > 0.0 && clock_ - lastCommand_ > p_.commandTimeout) {
            duty_ = 0.0; // the driver's own timeout: no fresh command, no drive
        }
        if (powerCut_) {
            duty_ = 0.0;
        }
        // PWM averaging: motor voltage = duty x battery voltage; the battery current is
        // duty x motor current plus the rest of the robot. Solved for the motor current:
        double i = (duty_ * (p_.batteryOpen - p_.otherLoad * p_.batteryRes) -
                    p_.torqueConst * omega_) /
                   (p_.resistance + p_.batteryRes * duty_ * duty_);
        if (powerCut_) {
            i = 0.0; // circuit open: no current, the wheel only coasts
        }
        if (p_.currentLimit > 0.0) {
            i = std::clamp(i, -p_.currentLimit, p_.currentLimit);
        }
        current_ = i;
        volts_ = p_.batteryOpen - p_.batteryRes * (duty_ * i + p_.otherLoad);
        const double drive = p_.torqueConst * i - p_.ripple * std::sin(angle_);
        double torque = drive - p_.viscous * omega_;
        if (std::abs(omega_) < 1e-3 && std::abs(drive) <= p_.coulomb) {
            omega_ = 0.0; // static friction holds the wheel
            torque = 0.0;
        } else {
            const double dir = (std::abs(omega_) < 1e-3) ? (drive > 0 ? 1.0 : -1.0)
                                                         : (omega_ > 0 ? 1.0 : -1.0);
            torque -= p_.coulomb * dir;
        }
        omega_ += kDt * torque / p_.inertia;
        angle_ += kDt * omega_;
        clock_ += kDt;
    }

    KitParams p_;
    Rng rng_;
    Rng refRng_;
    std::string label_;
    std::deque<std::pair<double, double>> pending_; // (time to apply, duty)
    double volts_;
    double duty_ = 0.0, omega_ = 0.0, angle_ = 1000.0, current_ = 0.0;
    double clock_ = 0.0, nominal_ = 0.0;
    double stamp_ = 0.0, voltsAtStamp_ = 0.0, referenceAtStamp_ = 0.0;
    double lastCommand_ = 0.0;
    bool powerCut_ = false;
    long samples_ = 0;
};

} // namespace br08
