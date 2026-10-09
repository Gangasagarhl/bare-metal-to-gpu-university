// timerq.h - F3-25: a timer queue: pending timeouts kept sorted by deadline, so the earliest
// is always at the front and the hardware timer only needs programming for that one.
// Pure logic with a fixed capacity (no heap in interrupt context); tested by clock_host.cpp.
#pragma once
#include <cstdint>

struct Timer {
    uint64_t deadline_ns = 0;
    uint64_t period_ns = 0;               // 0 = one-shot, otherwise re-armed after each expiry
    void (*fn)(Timer&) = nullptr;
    void* ctx = nullptr;
    bool queued = false;
};

template <int Capacity>
class TimerQueue {
public:
    bool add(Timer& t)                     // insertion keeps the array sorted by deadline
    {
        if (n_ == Capacity || t.queued) {
            return false;
        }
        int i = n_++;
        while (i > 0 && q_[i - 1]->deadline_ns > t.deadline_ns) {
            q_[i] = q_[i - 1];
            --i;
        }
        q_[i] = &t;
        t.queued = true;
        return true;
    }
    bool remove(Timer& t)
    {
        for (int i = 0; i < n_; ++i) {
            if (q_[i] == &t) {
                for (int j = i; j + 1 < n_; ++j) {
                    q_[j] = q_[j + 1];
                }
                --n_;
                t.queued = false;
                return true;
            }
        }
        return false;
    }
    // Runs every timer whose deadline has passed; periodic timers go back in. Returns how many ran.
    int expire(uint64_t now_ns)
    {
        int ran = 0;
        while (n_ > 0 && q_[0]->deadline_ns <= now_ns) {
            Timer& t = *q_[0];
            remove(t);
            if (t.period_ns != 0) {
                t.deadline_ns += t.period_ns;
                if (t.deadline_ns <= now_ns) {        // we fell behind: skip missed periods
                    t.deadline_ns = now_ns + t.period_ns;
                }
                add(t);
            }
            ++ran;
            t.fn(t);
        }
        return ran;
    }
    bool empty() const { return n_ == 0; }
    uint64_t next_deadline() const { return n_ > 0 ? q_[0]->deadline_ns : UINT64_MAX; }
    int size() const { return n_; }

private:
    Timer* q_[Capacity] = {};
    int n_ = 0;
};
