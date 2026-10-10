// uctl.h - F9-50 Listing 1: a miniature control framework with the SHAPE of ros2_control,
// written for this course because the build container has no ROS 2. Names are our own.
//   * Hardware exports named state interfaces (read by controllers) and command interfaces
//     (written by controllers). Each interface is a name plus a pointer to a double that the
//     hardware owns, so the loop exchanges values without copying or allocating.
//   * A Controller claims the interfaces it needs by name when it is configured.
//   * The ControllerManager runs the cycle: hardware read -> controllers update -> write.
#pragma once
#include <stdexcept>
#include <string>
#include <vector>

namespace uctl {

struct StateInterface {          // read-only view of one value the hardware measured
    std::string name;            // "<joint>/<quantity>", for example "left_wheel/velocity"
    const double* value;
};

struct CommandInterface {        // one value a controller writes and the hardware sends out
    std::string name;
    double* value;
};

class Hardware {
public:
    virtual ~Hardware() = default;
    virtual void init() = 0;                                   // allocate, open, configure
    virtual void activate() = 0;                               // safe start: zero commands
    virtual std::vector<StateInterface> exportStates() = 0;
    virtual std::vector<CommandInterface> exportCommands() = 0;
    virtual void read(double dt) = 0;                          // device -> state values
    virtual void write(double dt) = 0;                         // command values -> device
};

class Controller {
public:
    virtual ~Controller() = default;
    virtual std::vector<std::string> wantedStates() const = 0;
    virtual std::vector<std::string> wantedCommands() const = 0;
    // called once, outside the loop, with the claimed interfaces in the order asked for
    virtual void configure(std::vector<const double*> states, std::vector<double*> commands) = 0;
    virtual void update(double t, double dt) = 0;              // must not allocate or block
};

class ControllerManager {
public:
    explicit ControllerManager(Hardware& hw) : hw_(hw) {}

    void addController(Controller& c) { controllers_.push_back(&c); }

    // Everything that can fail or allocate happens here, before the real-time loop.
    void configure()
    {
        hw_.init();
        states_ = hw_.exportStates();
        commands_ = hw_.exportCommands();
        std::vector<bool> claimed(commands_.size(), false);
        for (Controller* c : controllers_) {
            std::vector<const double*> s;
            std::vector<double*> cmd;
            for (const std::string& n : c->wantedStates()) { s.push_back(findState(n)); }
            for (const std::string& n : c->wantedCommands()) {
                const std::size_t i = findCommand(n);
                if (claimed[i]) { throw std::runtime_error("command interface claimed twice: " + n); }
                claimed[i] = true;
                cmd.push_back(commands_[i].value);
            }
            c->configure(std::move(s), std::move(cmd));
        }
        hw_.activate();
    }

    void cycle(double t, double dt)                            // one period of the loop
    {
        hw_.read(dt);
        for (Controller* c : controllers_) { c->update(t, dt); }
        hw_.write(dt);
    }

private:
    const double* findState(const std::string& n) const
    {
        for (const StateInterface& s : states_) { if (s.name == n) { return s.value; } }
        throw std::runtime_error("no state interface " + n);
    }
    std::size_t findCommand(const std::string& n) const
    {
        for (std::size_t i = 0; i < commands_.size(); ++i) { if (commands_[i].name == n) { return i; } }
        throw std::runtime_error("no command interface " + n);
    }

    Hardware& hw_;
    std::vector<Controller*> controllers_;
    std::vector<StateInterface> states_;
    std::vector<CommandInterface> commands_;
};

}  // namespace uctl
