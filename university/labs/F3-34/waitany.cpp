// waitany.cpp - F3-34: a kernel-style "wait for any of several events" primitive, modelled in host
// C++ (one dispatcher lock, one wait block per object being waited on), then the host kernel's own
// multi-wait call, poll(), on three real pipes for comparison.
#include <condition_variable>
#include <cstdio>
#include <mutex>
#include <poll.h>
#include <thread>
#include <unistd.h>
#include <vector>

std::mutex dispatcher;                   // protects every event and every wait block below

struct Waiter                            // one sleeping thread
{
    std::condition_variable cv;
    int fired = -1;                      // which of its objects woke it
};

struct WaitBlock { Waiter* w; int index; };   // "waiter w waits on this object as its object #index"

class Event
{
public:
    explicit Event(bool auto_reset) : auto_reset_(auto_reset) {}

    void set()
    {
        std::lock_guard<std::mutex> g(dispatcher);
        for (WaitBlock& wb : blocks_) {
            if (wb.w->fired >= 0) continue;           // already woken by another object
            wb.w->fired = wb.index;
            wb.w->cv.notify_one();
            if (auto_reset_) return;                  // auto-reset: the signal goes to one waiter only
        }
        signaled_ = true;                             // nobody took it: remember it
    }

private:
    friend int wait_any(std::vector<Event*>& evs);
    bool auto_reset_;
    bool signaled_ = false;
    std::vector<WaitBlock> blocks_;
};

int wait_any(std::vector<Event*>& evs)
{
    std::unique_lock<std::mutex> lk(dispatcher);
    for (std::size_t i = 0; i < evs.size(); ++i) {   // already signaled? take it without sleeping
        if (evs[i]->signaled_) {
            if (evs[i]->auto_reset_) evs[i]->signaled_ = false;
            return static_cast<int>(i);
        }
    }
    Waiter me;
    for (std::size_t i = 0; i < evs.size(); ++i) evs[i]->blocks_.push_back(WaitBlock{&me, static_cast<int>(i)});
    me.cv.wait(lk, [&] { return me.fired >= 0; });
    for (Event* e : evs) {                            // unhook from every object before returning
        auto& b = e->blocks_;
        for (std::size_t k = 0; k < b.size(); ++k) if (b[k].w == &me) { b.erase(b.begin() + static_cast<long>(k)); break; }
    }
    return me.fired;
}

int main()
{
    std::printf("== model: one thread waits on three auto-reset events ==\n");
    const int rounds = 20000;
    std::vector<Event> ev, ack;
    for (int i = 0; i < 3; ++i) { ev.emplace_back(true); ack.emplace_back(true); }
    std::vector<Event*> wanted{&ev[0], &ev[1], &ev[2]};
    std::vector<std::thread> senders;
    for (int i = 0; i < 3; ++i) {
        senders.emplace_back([&, i] {
            std::vector<Event*> mine{&ack[static_cast<std::size_t>(i)]};
            for (int k = 0; k < rounds; ++k) { ev[static_cast<std::size_t>(i)].set(); wait_any(mine); }
        });
    }
    int count[3] = {0, 0, 0};
    for (int k = 0; k < 3 * rounds; ++k) {
        int which = wait_any(wanted);
        ++count[which];
        ack[static_cast<std::size_t>(which)].set();     // let that sender continue
    }
    for (auto& t : senders) t.join();
    std::printf("wake-ups per event: %d %d %d (expected %d each)\n", count[0], count[1], count[2], rounds);
    bool ok = count[0] == rounds && count[1] == rounds && count[2] == rounds;

    std::printf("== host kernel: poll() on three pipes ==\n");
    int p[3][2];
    for (auto& fd : p) if (pipe(fd) != 0) return 1;
    if (write(p[2][1], "x", 1) != 1 || write(p[0][1], "y", 1) != 1) return 1;
    close(p[1][1]);                                   // pipe 1 loses its only writer
    pollfd fds[3];
    for (int i = 0; i < 3; ++i) fds[i] = pollfd{p[i][0], POLLIN, 0};
    int n = poll(fds, 3, 1000);
    std::printf("poll returned %d\n", n);
    for (int i = 0; i < 3; ++i)
        std::printf("  pipe %d: %s%s%s\n", i, fds[i].revents & POLLIN ? "POLLIN " : "",
                    fds[i].revents & POLLHUP ? "POLLHUP " : "", fds[i].revents == 0 ? "nothing" : "");
    std::printf("wait-any model: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
