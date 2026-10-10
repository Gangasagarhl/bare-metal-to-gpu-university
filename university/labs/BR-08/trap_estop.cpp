// BR-08 Listing 8, trap 3: testing without an e-stop (or with propellers on).
// Part A: the control program hangs during an acceleration, and the motor driver keeps the
// last command. Four set-ups: nothing else; a watchdog inside the same program; a command
// timeout in the motor driver; a hardware emergency stop pressed by the supervisor.
// Part B: an arming interlock that refuses to drive motors until the bench record says the
// propellers are off and the e-stop was tested (the record is the university's rule, BR-08).
#include "bench.hpp"
#include "kit_model.hpp"
#include <cmath>
#include <format>
#include <iostream>
#include <string>
#include <vector>

namespace {

constexpr double kPeriod = 0.01;
constexpr double kHangAt = 0.15;      // s after the loop started: the program stops here
constexpr double kReaction = 1.0;     // s, exercise value: replace it with your own measurement
constexpr double kRunSeconds = 4.0;

struct Case
{
    const char* name;
    double driverTimeout; // s, 0 = none
    bool supervisorEstop;
};

void runCase(const Case& c)
{
    br08::KitParams p;
    p.commandTimeout = c.driverTimeout;
    br08::KitWorld kit{p};
    rb::PidConfig cfg;
    cfg.kp = br08::kSimGains.kp;
    cfg.ki = br08::kSimGains.ki;
    cfg.period = kPeriod;
    cfg.outMin = -12.0;
    cfg.outMax = 12.0;
    rb::Pid pid = rb::Pid::create(cfg).value();
    std::uint16_t before = kit.readEncoder();
    double setpoint = 0, lastDuty = 0, watchdogLastRan = 0, atHang = 0, peak = 0;
    double stoppedAt = -1, estopAt = -1;
    std::vector<std::pair<double, double>> marks; // (time, true speed)
    const int n = static_cast<int>(kRunSeconds / kPeriod);
    for (int k = 0; k < n; ++k) {
        const double t = k * kPeriod;
        const bool hung = t >= kHangAt;
        if (!hung) { // the control program, as in bench.hpp
            setpoint = std::min(250.0, setpoint + 3000.0 * kPeriod);
            const std::uint16_t now = kit.readEncoder();
            const double measured = speedFromCounts(now, before, 1024.0, kPeriod);
            before = now;
            lastDuty = pid.update(setpoint, measured) / 12.0;
            kit.setDuty(lastDuty);
            watchdogLastRan = t; // an in-process watchdog runs only while the program runs
        } else if (atHang == 0) {
            atHang = kit.trueSpeed();
        }
        if (c.supervisorEstop && hung && estopAt < 0 && t >= kHangAt + kReaction) {
            kit.emergencyStop();
            estopAt = t;
            marks.push_back({t, kit.trueSpeed()});
        }
        kit.advance(kPeriod);
        peak = std::max(peak, kit.trueSpeed());
        if (hung && stoppedAt < 0 && std::abs(kit.trueSpeed()) < 5.0) {
            stoppedAt = t + kPeriod;
        }
    }
    std::cout << std::format("{}\n  last command before the hang: duty {:.2f}; true speed at the "
                             "hang {:.0f} rad/s\n",
                             c.name, lastDuty, atHang);
    std::cout << std::format("  in-process watchdog last ran at t = {:.2f} s\n", watchdogLastRan);
    if (estopAt >= 0) {
        std::cout << std::format("  e-stop pressed at t = {:.2f} s, true speed then {:.0f} rad/s\n",
                                 estopAt, marks.front().second);
    }
    std::cout << std::format("  peak true speed {:.0f} rad/s; ", peak);
    if (stoppedAt >= 0) {
        std::cout << std::format("wheel below 5 rad/s at t = {:.2f} s\n", stoppedAt);
    } else {
        std::cout << std::format("still turning at {:.0f} rad/s when the run ended at {:.1f} s\n",
                                 kit.trueSpeed(), kRunSeconds);
    }
}

// Part B: the record the supervisor fills in before any powered step (BR-08 safety line).
struct BenchRecord
{
    bool propsOff, estopTested, supervisorPresent, learnerAtLeastL3, rulesRecorded;
};

enum class Request { MotorTest, ClosedLoopBench, OutdoorFlight };

std::string decide(const BenchRecord& r, Request q)
{
    if (!r.estopTested) {
        return "REFUSED: e-stop not tested";
    }
    if (!r.learnerAtLeastL3 && !r.supervisorPresent) {
        return "REFUSED: learner below L3 without supervision";
    }
    if ((q == Request::MotorTest || q == Request::ClosedLoopBench) && !r.propsOff) {
        return "REFUSED: bench step with propellers on";
    }
    if (q == Request::OutdoorFlight && !r.rulesRecorded) {
        return "REFUSED: local drone rules not read and recorded";
    }
    if (q == Request::OutdoorFlight && !r.supervisorPresent) {
        return "REFUSED: no supervisor for the flight";
    }
    return "allowed";
}

} // namespace

int main()
{
    std::cout << std::format("Part A: the program hangs {:.2f} s into an acceleration to 250 "
                             "rad/s\n\n",
                             kHangAt);
    for (const Case& c : {Case{"A. nothing but the program", 0.0, false},
                          Case{"B. watchdog inside the same program", 0.0, false},
                          Case{"C. command timeout of 0.05 s in the motor driver", 0.05, false},
                          Case{"D. hardware e-stop, supervisor reacts after 1.0 s", 0.0, true}}) {
        runCase(c);
    }

    std::cout << "\nPart B: arming interlock (props off, e-stop tested, supervision, rules)\n";
    struct Row
    {
        const char* what;
        BenchRecord r;
        Request q;
    };
    const Row rows[] = {
        {"motor test, all boxes ticked        ", {true, true, true, true, false},
         Request::MotorTest},
        {"motor test, propellers still on     ", {false, true, true, true, false},
         Request::MotorTest},
        {"closed loop on bench, e-stop untried", {true, false, true, true, false},
         Request::ClosedLoopBench},
        {"bench, L2 learner, nobody watching  ", {true, true, false, false, false},
         Request::ClosedLoopBench},
        {"outdoor flight, rules not recorded  ", {false, true, true, true, false},
         Request::OutdoorFlight},
        {"outdoor flight, everything recorded ", {false, true, true, true, true},
         Request::OutdoorFlight},
    };
    for (const Row& row : rows) {
        std::cout << "  " << row.what << " -> " << decide(row.r, row.q) << '\n';
    }
    return 0;
}
