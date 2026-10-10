// estop_timeline.cpp - F11-21: "the arm moved after e-stop", replayed on a model of one CPU.
// Two threads share the CPU in 10 us steps (virtual time, so every run is identical):
//   control (real-time, highest priority): released every 1 ms, runs 200 us, moves the arm joint
//   comms   (not real-time): works through a FIFO queue of jobs (log flushes, messages)
// The e-stop button drops the hard-wired chain; the safety relay removes drive power after its
// fixed delay; software must ramp the arm to rest before then. When the relay opens, the drive
// loses power and the joint's brake holds it.
// Input: one line "where log_flush_ms relay_delay_ms"
//   where = comms : the e-stop input is delivered as a message that the comms thread handles
//   where = control : the control thread reads the e-stop input itself at the start of each cycle
// All durations are the course's illustrative values, not measurements of any robot.
#include <cstdio>
#include <deque>
#include <iostream>
#include <string>

struct Job {
    std::string name;
    long remaining_us;
};

int main()
{
    std::string where;
    long flush_ms = 0, relay_ms = 0;
    if (!(std::cin >> where >> flush_ms >> relay_ms)) {
        std::cerr << "usage: where log_flush_ms relay_delay_ms\n";
        return 2;
    }
    const long estop_us = 1'020'000;                 // the button is pressed at 1020 ms
    const long relay_us = estop_us + relay_ms * 1000;
    const double cruise = 60.0;                       // deg/s while following the trajectory
    const double decel = 0.6;                         // deg/s per 1 ms cycle (controlled ramp)
    std::deque<Job> queue;
    bool stop_flag = false, power = true, logged_rest = false, cut_while_moving = false;
    double angle = 0.0, speed = 0.0, angle_at_estop = 0.0;
    long control_left = 0;
    std::printf("[  time ms ] thread   event (e-stop handled in: %s)\n", where.c_str());
    for (long t = 0; t <= 1'800'000; t += 10) {
        if (t == 1'000'000) {
            queue.push_back({"log_flush", flush_ms * 1000});
            std::printf("[%10.3f] comms    job queued: log_flush (%ld ms of work)\n", t / 1e3,
                        flush_ms);
        }
        if (t == estop_us) {
            angle_at_estop = angle;
            std::printf("[%10.3f] hw       E-STOP pressed; safety relay will cut drive power at"
                        " %.3f ms\n", t / 1e3, relay_us / 1e3);
            if (where == "comms") {
                queue.push_back({"estop_msg", 50});
                std::printf("[%10.3f] comms    job queued: estop_msg (queue length %zu)\n",
                            t / 1e3, queue.size());
            }
        }
        if (t == relay_us) {
            power = false;
            cut_while_moving = speed > 0.0;
            std::printf("[%10.3f] hw       relay opened: drive power OFF (arm speed was %.1f"
                        " deg/s)\n", t / 1e3, speed);
            speed = 0.0;                              // drive off; the joint brake holds
        }
        if (t % 1000 == 0) {                          // control thread released
            control_left = 200;
            const bool estop_seen = (where == "control") ? (t >= estop_us) : stop_flag;
            if (power) {
                speed = estop_seen ? (speed > decel ? speed - decel : 0.0) : cruise;
            }
            angle += speed / 1000.0;
            if (t >= estop_us - 40'000 && t <= relay_us && t % 20'000 == 0) {
                std::printf("[%10.3f] control  cycle: angle %7.3f deg, speed %5.1f deg/s%s\n",
                            t / 1e3, angle, speed, estop_seen ? "  (stopping)" : "");
            }
            if (estop_seen && speed == 0.0 && !logged_rest) {
                logged_rest = true;
                std::printf("[%10.3f] control  arm at rest; moved %.3f deg after the e-stop\n",
                            t / 1e3, angle - angle_at_estop);
            }
        }
        if (control_left > 0) {                       // the real-time thread runs first
            control_left -= 10;
        } else if (!queue.empty()) {                  // otherwise the comms thread runs
            Job& j = queue.front();
            j.remaining_us -= 10;
            if (j.remaining_us <= 0) {
                std::printf("[%10.3f] comms    job done: %s\n", (t + 10) / 1e3, j.name.c_str());
                if (j.name == "estop_msg") {
                    stop_flag = true;
                    std::printf("[%10.3f] comms    stop_flag set for the control thread"
                                " (%.3f ms after the press)\n", (t + 10) / 1e3,
                                (t + 10 - estop_us) / 1e3);
                }
                queue.pop_front();
            }
        }
    }
    std::printf("summary: arm moved %.3f deg after the e-stop; %s\n", angle - angle_at_estop,
                cut_while_moving ? "the relay cut power while it was still moving"
                                 : "software brought it to rest before the relay");
    return 0;
}
