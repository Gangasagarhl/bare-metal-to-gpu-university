// modes.cpp - F10-28 Listing 2: flight modes as classes. Each mode says what it needs
// (here: a position estimate) and refuses to start without it; the vehicle keeps the old
// mode when a change is refused and logs why. The structure follows the pattern of the
// ArduPilot vehicle code (one class per mode, a common base, a mode-change call with a
// reason); class names, mode names and the fallback policy below are exercise choices.
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

struct VehicleState {
    bool positionOk = false;     // the estimator has a usable position (GPS, etc.)
    bool armed = false;
};

class Mode {
public:
    virtual ~Mode() = default;
    virtual std::string name() const = 0;
    virtual bool requiresPosition() const = 0;
    bool canEnter(const VehicleState& s, std::string& why) const
    {
        if (requiresPosition() && !s.positionOk) {
            why = "needs a position estimate";
            return false;
        }
        return true;
    }
};

class Stabilize final : public Mode {
public:
    std::string name() const override { return "STABILIZE"; }
    bool requiresPosition() const override { return false; }
};
class AltHold final : public Mode {
public:
    std::string name() const override { return "ALT_HOLD"; }
    bool requiresPosition() const override { return false; }
};
class Loiter final : public Mode {
public:
    std::string name() const override { return "LOITER"; }
    bool requiresPosition() const override { return true; }
};
class Rtl final : public Mode {
public:
    std::string name() const override { return "RTL"; }
    bool requiresPosition() const override { return true; }
};
class Land final : public Mode {
public:
    std::string name() const override { return "LAND"; }
    bool requiresPosition() const override { return false; }
};

class Vehicle {
public:
    Vehicle()
    {
        modes_.push_back(std::make_unique<Stabilize>());
        modes_.push_back(std::make_unique<AltHold>());
        modes_.push_back(std::make_unique<Loiter>());
        modes_.push_back(std::make_unique<Rtl>());
        modes_.push_back(std::make_unique<Land>());
        current_ = modes_[0].get();
    }
    bool setMode(const std::string& wanted, const std::string& reason, int t)
    {
        const Mode* m = find(wanted);
        std::string why;
        if (m == nullptr) {
            why = "no such mode";
        } else if (m->canEnter(state, why)) {
            std::printf("%5d ms  MODE %-9s -> %-9s reason: %s\n", t, current_->name().c_str(),
                        m->name().c_str(), reason.c_str());
            current_ = m;
            return true;
        }
        std::printf("%5d ms  MODE %-9s refused (%s), staying in %s; asked by: %s\n", t,
                    wanted.c_str(), why.c_str(), current_->name().c_str(), reason.c_str());
        return false;
    }
    // Called every loop: a mode that needs position cannot continue without it.
    void checkModeStillValid(int t)
    {
        if (current_->requiresPosition() && !state.positionOk) {
            std::printf("%5d ms  EVENT position estimate lost while in %s\n", t,
                        current_->name().c_str());
            if (!setMode("LAND", "failsafe: position lost", t)) {
                setMode("ALT_HOLD", "failsafe: position lost", t);
            }
        }
    }
    VehicleState state;

private:
    const Mode* find(const std::string& n) const
    {
        for (const auto& m : modes_) {
            if (m->name() == n) {
                return m.get();
            }
        }
        return nullptr;
    }
    std::vector<std::unique_ptr<Mode>> modes_;
    const Mode* current_;
};

int main()
{
    Vehicle v;
    v.setMode("LOITER", "pilot switch", 0);            // no position yet: refused
    v.state.positionOk = true;                         // GPS lock, estimator converged
    v.setMode("LOITER", "pilot switch", 4000);
    v.setMode("AUTO", "ground station", 6000);         // not built into this vehicle
    v.state.positionOk = false;                        // GPS glitch
    v.checkModeStillValid(9000);
    v.setMode("RTL", "pilot switch", 9500);            // cannot return without position
    return 0;
}
