// safety.cpp - F9-69: a safety supervisor for one drive, simulated in 1 ms steps.
// The control task computes a speed command and kicks the watchdog. The supervisor, which is
// independent of the control task, watches the kicks and the e-stop and owns the drive:
//   READY    -> the drive follows the control command
//   STOPPING -> controlled stop: the command ramps down at the deceleration limit
//   LOCKED   -> drive disabled (command 0, enable off) until the e-stop is released AND an
//               operator presses reset
// The scenario (stdin) lists timed events. Every step checks the safety invariants.
#include <algorithm>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

enum class State { Ready, Stopping, Locked };

const char* name(State s)
{
    switch (s) {
    case State::Ready: return "READY";
    case State::Stopping: return "STOPPING";
    case State::Locked: return "LOCKED";
    }
    return "?";
}

struct Config {
    int watchdog_timeout_ms = 50;   // no kick for longer than this is a fault
    int decel_mm_s_per_ms = 2;      // controlled-stop ramp: 2 mm/s per ms = 2 m/s^2
    int cruise_mm_s = 500;          // what the control task asks for when healthy
};

int main()
{
    Config cfg;
    std::multimap<int, std::string> events;   // time in ms -> event line
    int end_ms = 0;
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        std::istringstream in(line);
        int t = 0;
        std::string what;
        in >> t >> what;
        if (what == "end") {
            end_ms = t;
        } else {
            events.emplace(t, line.substr(line.find(what)));
        }
    }

    State state = State::Ready;
    bool estop = false;
    bool kick_from_control = true;      // false: a timer kicks, whatever the control task does
    int stalled_until = -1;             // the control task is stuck until this time
    int last_kick = 0;
    int control_cmd = 0;                // last command the control task produced
    int last_control_update = 0;
    int drive_cmd = 0;
    bool enable = true;
    int longest_stale = 0;
    int violations = 0;
    int fault_time = -1;

    std::cout << "watchdog timeout " << cfg.watchdog_timeout_ms << " ms, stop ramp "
              << cfg.decel_mm_s_per_ms << " mm/s per ms, cruise " << cfg.cruise_mm_s << " mm/s\n";
    for (int t = 0; t <= end_ms; ++t) {
        auto [first, last] = events.equal_range(t);
        for (auto it = first; it != last; ++it) {
            std::istringstream in(it->second);
            std::string what;
            int arg = 0;
            in >> what >> arg;
            std::cout << "t=" << t << " ms  event: " << it->second << '\n';
            if (what == "estop_press") {
                estop = true;
            } else if (what == "estop_release") {
                estop = false;
            } else if (what == "stall") {
                stalled_until = t + arg;
            } else if (what == "kick_source") {
                kick_from_control = it->second.find("control") != std::string::npos;
            } else if (what == "reset") {
                bool healthy = t - last_kick <= cfg.watchdog_timeout_ms;
                if (state == State::Locked && !estop && healthy) {
                    state = State::Ready;
                    enable = true;
                    std::cout << "t=" << t << " ms  reset accepted: READY\n";
                } else {
                    std::cout << "t=" << t << " ms  reset REFUSED (state " << name(state)
                              << (estop ? ", e-stop still pressed" : "")
                              << (healthy ? "" : ", watchdog not healthy") << ")\n";
                }
            }
        }

        // the control task (1 ms period): ramps up by 1 mm/s per ms to cruise speed; it reads
        // the supervisor's state and restarts its ramp from 0 whenever the drive is not READY
        bool control_runs = t >= stalled_until;
        if (control_runs) {
            control_cmd = state == State::Ready ? std::min(cfg.cruise_mm_s, control_cmd + 1) : 0;
            last_control_update = t;
            if (kick_from_control) {
                last_kick = t;
            }
        }
        if (!kick_from_control) {
            last_kick = t;              // the timer kicks every step
        }

        // the supervisor
        bool watchdog_fault = t - last_kick > cfg.watchdog_timeout_ms;
        if (state == State::Ready && (estop || watchdog_fault)) {
            state = State::Stopping;
            fault_time = t;
            std::cout << "t=" << t << " ms  FAULT: " << (estop ? "e-stop" : "watchdog (no kick for ")
                      << (estop ? "" : std::to_string(t - last_kick) + " ms)")
                      << " -> STOPPING from " << drive_cmd << " mm/s\n";
        }
        int before = drive_cmd;
        if (state == State::Ready) {
            drive_cmd = control_cmd;    // a stuck control task leaves its last value here
            longest_stale = std::max(longest_stale, t - last_control_update);
        } else if (state == State::Stopping) {
            drive_cmd = std::max(0, drive_cmd - cfg.decel_mm_s_per_ms);
            if (drive_cmd == 0) {
                state = State::Locked;
                enable = false;
                std::cout << "t=" << t << " ms  stopped after " << t - fault_time
                          << " ms -> LOCKED (drive disabled)\n";
            }
        } else {
            drive_cmd = 0;
        }

        // invariants, checked every step
        if (!enable && drive_cmd != 0) {
            ++violations;
        }
        if (state != State::Ready && drive_cmd > before) {
            ++violations;               // never speed up outside READY
        }
        if (drive_cmd - before > 1) {
            ++violations;               // never jump: at most the 1 mm/s per ms ramp
        }
    }
    std::cout << "end at t=" << end_ms << " ms: state " << name(state) << ", drive " << drive_cmd
              << " mm/s, enable " << (enable ? "on" : "off") << '\n';
    std::cout << "longest time the drive followed a command the control task had not refreshed: "
              << longest_stale << " ms\n";
    std::cout << "invariant violations: " << violations << '\n';
    return violations == 0 ? 0 : 1;
}
