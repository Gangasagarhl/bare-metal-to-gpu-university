// mp6_safety.hpp - MP6 starter lab (M1): the software safety supervisor between the navigator
// and the motor driver, after F9-69 and F11-21. It is a TEACHING MODEL: on the real robot the
// e-stop is a hard-wired chain with no software in its path; this supervisor only adds a
// latched software stop, a command watchdog and a speed clamp in front of the drive.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace mp6 {

struct Cmd { double v = 0.0, w = 0.0; };

struct SafetyParams
{
    double speedLimit = 0.30;   // m/s, |v| never above this
    double rotLimit = 1.00;     // rad/s
    double watchdog = 0.25;     // s, a command older than this is a fault
    // Mutant switches: each one removes one safety mechanism (the suite must notice).
    bool clamp = true;
    bool watchdogOn = true;
    bool latchEstop = true;
};

enum class SafeState { Ready, Locked };

class Supervisor
{
public:
    explicit Supervisor(const SafetyParams& p) : p_(p) {}

    // Called every 10 ms. `stamp` is the time the navigator produced `cmd`.
    // Returns the command that goes to the motor driver; `event` gets a line when the state changes.
    Cmd update(const Cmd& cmd, double stamp, bool estop, bool resetPulse, double now, std::string& event)
    {
        event.clear();
        if (estop && state_ == SafeState::Ready) {
            lock("e-stop pressed", event);
        } else if (p_.watchdogOn && state_ == SafeState::Ready && now - stamp > p_.watchdog) {
            char buf[96];
            std::snprintf(buf, sizeof buf, "watchdog: newest command is %.2f s old", now - stamp);
            lock(buf, event);
        } else if (state_ == SafeState::Locked && !p_.latchEstop && reason_ == "e-stop pressed" && !estop) {
            state_ = SafeState::Ready;                       // the mutant: release alone re-enables the drive
            event = "e-stop released -> READY (no reset needed)";
        } else if (state_ == SafeState::Locked && resetPulse) {
            if (estop) {
                event = "reset refused: e-stop still pressed";
            } else if (now - stamp > p_.watchdog) {
                event = "reset refused: no fresh command from the navigator";
            } else {
                state_ = SafeState::Ready;
                event = "reset accepted -> READY";
            }
        }
        if (state_ == SafeState::Locked) { return {}; }
        Cmd out = cmd;
        if (p_.clamp) {
            out.v = std::clamp(out.v, -p_.speedLimit, p_.speedLimit);
            out.w = std::clamp(out.w, -p_.rotLimit, p_.rotLimit);
        }
        return out;
    }

    SafeState state() const { return state_; }

private:
    void lock(const std::string& why, std::string& event)
    {
        state_ = SafeState::Locked;
        reason_ = why;
        event = "SAFE STOP (latched): " + why;
    }

    SafetyParams p_;
    SafeState state_ = SafeState::Ready;
    std::string reason_;
};

}  // namespace mp6
