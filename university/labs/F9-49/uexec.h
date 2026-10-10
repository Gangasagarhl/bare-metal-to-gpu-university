// uexec.h - F9-49 Listing 1: a miniature executor, written for this course to show the ideas
// of ROS 2 executors and callback groups WITHOUT ROS 2 (which the build container lacks).
// It is not rclcpp: names and behaviour are our own simplification.
//   * A CallbackGroup holds periodic timers whose callbacks must never run at the same time.
//   * An Executor owns ONE thread and serves the groups added to it: it sleeps until the
//     earliest timer is due, then runs every due callback, in the order they were added.
//   * Every callback run is recorded in a trace buffer allocated before spinning.
#pragma once
#include <pthread.h>
#include <sched.h>
#include <time.h>

#include <cstdio>
#include <cstring>
#include <functional>
#include <string>
#include <thread>
#include <vector>

namespace uexec {

inline long long nowNs()
{
    timespec t{};
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1'000'000'000LL + t.tv_nsec;
}

struct TraceEvent {
    int timer;            // index into Executor::names
    long long release;    // when the timer was due
    long long start;      // when its callback started
    long long end;        // when its callback returned
};

struct Timer {
    std::string name;
    long long periodNs;
    std::function<void()> callback;
    long long next = 0;   // next release time
    int id = 0;
};

struct CallbackGroup {
    std::vector<Timer> timers;
    void addTimer(std::string name, long long periodNs, std::function<void()> cb)
    {
        timers.push_back(Timer{std::move(name), periodNs, std::move(cb)});
    }
};

class Executor {
public:
    explicit Executor(std::string name, int fifoPriority = 0)
        : name_(std::move(name)), prio_(fifoPriority) {}

    void add(CallbackGroup& g) { groups_.push_back(&g); }

    // Spin for `durationNs` on a new thread; trace capacity is reserved before spinning.
    void start(long long t0, long long durationNs)
    {
        for (CallbackGroup* g : groups_) {
            for (Timer& t : g->timers) {
                t.id = static_cast<int>(names.size());
                names.push_back(t.name);
                t.next = t0 + t.periodNs;
            }
        }
        std::size_t cap = 0;
        for (CallbackGroup* g : groups_) {
            for (Timer& t : g->timers) { cap += static_cast<std::size_t>(durationNs / t.periodNs + 2); }
        }
        trace.reserve(cap);
        thread_ = std::thread([this, t0, durationNs] { spin(t0 + durationNs); });
    }
    void join() { thread_.join(); }

    std::vector<TraceEvent> trace;
    std::vector<std::string> names;
    std::string policy = "SCHED_OTHER";

private:
    void spin(long long stopAt)
    {
        if (prio_ > 0) {
            sched_param sp{};
            sp.sched_priority = prio_;
            const int err = pthread_setschedparam(pthread_self(), SCHED_FIFO, &sp);
            policy = err == 0 ? "SCHED_FIFO " + std::to_string(prio_) : "SCHED_OTHER (refused)";
        }
        for (;;) {
            long long due = stopAt;                       // earliest next release
            for (CallbackGroup* g : groups_) {
                for (Timer& t : g->timers) { due = t.next < due ? t.next : due; }
            }
            if (due >= stopAt) { return; }
            timespec r{};
            r.tv_sec = static_cast<time_t>(due / 1'000'000'000LL);
            r.tv_nsec = static_cast<long>(due % 1'000'000'000LL);
            clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &r, nullptr);
            for (CallbackGroup* g : groups_) {            // run every due callback, in order
                for (Timer& t : g->timers) {
                    if (t.next > nowNs()) { continue; }
                    const long long s = nowNs();
                    t.callback();
                    if (trace.size() < trace.capacity()) {
                        trace.push_back(TraceEvent{t.id, t.next, s, nowNs()});
                    }
                    t.next += t.periodNs;                 // absolute releases: no drift
                }
            }
        }
    }

    std::string name_;
    int prio_;
    std::vector<CallbackGroup*> groups_;
    std::thread thread_;
};

}  // namespace uexec
