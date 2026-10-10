// hitl_delay.cpp - what link latency does to the control loop.
// On a real HITL bench the flight controller runs on its own clock, so the time a sensor
// frame needs to reach it and the time a command needs to come back are added to the loop.
// Here a delay line of D milliseconds (sensor path + command path together) is inserted
// between the course flight software and the course world; the vehicle hovers at 10 m,
// then gets a 10 m step to the north. Deterministic: same output on every run.
#include "../F10-33/dronesim.hpp"

#include <cmath>
#include <cstdio>
#include <deque>

struct StepResult
{
    double overshoot, settle, late_amp;
    bool crashed;
};

StepResult step_test(int delay_ms)
{
    dn::World w;
    dn::Fsw f;
    dn::Sensors s = w.sense();
    f.arm(s, {});                 // no mission: take off, then HOLD
    const int d = delay_ms / 10;  // steps of 10 ms
    std::deque<dn::Command> line(static_cast<std::size_t>(d), dn::Command{{0, 0, 0}, true});
    double t_step = -1, max_x = 0, settle = -1, late_amp = 0;
    while (w.t < 60.0 && !w.crashed) {
        s = w.sense();
        line.push_back(f.step(s));
        const dn::Command c = line.front();
        line.pop_front();
        w.step(c);
        if (t_step < 0 && f.mode == dn::Mode::Hold && w.t > 12.0) {
            t_step = w.t;
            f.sp = {10, 0, 10};  // the step: 10 m north
        }
        if (t_step > 0) {
            max_x = std::max(max_x, w.pos.x);
            if (std::fabs(w.pos.x - 10) > 0.5)
                settle = -1;
            else if (settle < 0)
                settle = w.t - t_step;
            if (w.t > 50.0) late_amp = std::max(late_amp, std::fabs(w.pos.x - 10));
        }
    }
    return {max_x - 10, settle, late_amp, w.crashed};
}

int main()
{
    std::printf("%10s %14s %14s %24s\n", "delay[ms]", "overshoot[m]", "settle[s]",
                "|x-10| after 38 s [m]");
    for (int d : {0, 20, 50, 100, 200, 300, 400, 500, 600}) {
        const StepResult r = step_test(d);
        if (r.crashed) {
            std::printf("%10d  crashed\n", d);
            continue;
        }
        if (r.settle < 0)
            std::printf("%10d %14.2f %14s %24.2f\n", d, r.overshoot, "never", r.late_amp);
        else
            std::printf("%10d %14.2f %14.2f %24.2f\n", d, r.overshoot, r.settle, r.late_amp);
    }
    std::printf("settle = time until |x-10| stays below 0.5 m\n");
    return 0;
}
