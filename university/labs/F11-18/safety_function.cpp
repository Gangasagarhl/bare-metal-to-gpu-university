// safety_function.cpp - F11-18: a control function and a separate safety function.
// One joint of the course robot's arm is simulated in 1 ms steps for 2 seconds.
//   control function : a speed controller (target + integral correction) that moves the joint;
//   safety function  : a speed monitor that requests the safe state when the measured speed
//                      stays above a limit for 3 consecutive milliseconds.
// Each scenario line on stdin: name gain stuck_A_at_ms monitor_sensor trace
//   gain           1.0 = correct controller, 2.0 = a controller bug that asks for twice the speed
//   stuck_A_at_ms  time at which encoder A's count freezes, so its speed reads 0 (-1 = never)
//   monitor_sensor A = the monitor reads the controller's encoder A, B = its own encoder B
//   trace          1 = print a line every 100 ms and at every state change
// All numbers are the course's own illustrative values, not those of any real product.
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>

enum class Safety { Ready, Stopping, Locked };

const char* name(Safety s)
{
    switch (s) {
    case Safety::Ready: return "READY";
    case Safety::Stopping: return "STOPPING";
    case Safety::Locked: return "LOCKED";
    }
    return "?";
}

struct Limits {
    double target = 150.0;      // deg/s the task asks for
    double motor_max = 400.0;   // deg/s the drive can reach
    double accel = 3.0;         // deg/s per ms the drive can change speed
    double trip = 200.0;        // safety function: speed limit in deg/s
    int debounce_ms = 3;        // above the limit this long -> trip
    double hazard = 300.0;      // above this speed the arm can injure (course's hazard H2)
    double ki = 2.0;            // integral gain of the controller, per second
};

struct Result {
    double max_speed = 0.0;
    int trip_ms = -1;
    bool hazard = false;
    Safety final_state = Safety::Ready;
};

Result run(const std::string& label, double gain, int stuck_at, char monitor, bool trace)
{
    const Limits lim;
    double v = 0.0;            // true joint speed, deg/s
    double integral = 0.0;     // controller state, deg
    double enc_a = 0.0;        // encoder A (motor side), read by the controller
    Safety st = Safety::Ready;
    int above = 0;
    Result r;
    for (int t = 0; t < 2000; ++t) {
        // sensors
        // healthy: A follows the true speed; frozen count: the speed computed from it is 0
        enc_a = (stuck_at < 0 || t < stuck_at) ? v : 0.0;
        const double enc_b = v;                // encoder B (joint side), independent of A
        // control function: runs every step, knows nothing about safety
        integral += (lim.target - enc_a) / 1000.0;
        double cmd = std::clamp(gain * lim.target + lim.ki * integral, 0.0, lim.motor_max);
        // safety function: separate code, separate state, owns the drive output
        const double seen = (monitor == 'B') ? enc_b : enc_a;
        const Safety before = st;
        if (st == Safety::Ready) {
            above = (seen > lim.trip) ? above + 1 : 0;
            if (above >= lim.debounce_ms) {
                st = Safety::Stopping;
                r.trip_ms = t;
            }
        }
        if (st != Safety::Ready) {
            cmd = 0.0;                         // controlled stop: ramp to zero at the accel limit
            if (st == Safety::Stopping && v <= 0.0) {
                st = Safety::Locked;           // drive enable off, latched
            }
        }
        // the drive follows the command at its acceleration limit
        v += std::clamp(cmd - v, -lim.accel, lim.accel);
        r.max_speed = std::max(r.max_speed, v);
        r.hazard = r.hazard || v > lim.hazard;
        if (trace && ((t % 100 == 0 && t <= 1000) || st != before)) {
            std::printf("  t=%4d ms  encA=%6.1f  encB(true)=%6.1f  cmd=%6.1f  %s\n",
                        t, enc_a, enc_b, cmd, name(st));
        }
    }
    r.final_state = st;
    std::printf("%-16s gain=%.1f stuckA=%5d monitor=%c | max speed %6.1f deg/s | trip %s",
                label.c_str(), gain, stuck_at, monitor, r.max_speed,
                r.trip_ms < 0 ? "never  " : "");
    if (r.trip_ms >= 0) {
        std::printf("%4d ms", r.trip_ms);
    }
    std::printf(" | final %-8s | %s\n", name(r.final_state),
                r.hazard ? "HAZARD: speed above 300 deg/s" : "no hazardous speed");
    return r;
}

int main()
{
    std::string line;
    int hazards = 0;
    while (std::getline(std::cin, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        std::istringstream in(line);
        std::string label;
        double gain = 1.0;
        int stuck = -1;
        char monitor = 'B';
        int trace = 0;
        if (!(in >> label >> gain >> stuck >> monitor >> trace)) {
            std::cerr << "bad scenario line: " << line << '\n';
            return 2;
        }
        hazards += run(label, gain, stuck, monitor, trace != 0).hazard ? 1 : 0;
    }
    std::printf("scenarios with a hazardous speed: %d\n", hazards);
    return 0;
}
