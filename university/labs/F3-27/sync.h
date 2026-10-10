// sync.h - synchronization primitives of the OS303 kernel (milestone B10).
// Spinlock: a ticket lock. Interrupts are the caller's choice: lock_irqsave() is the
// safe default; plain lock() is only for locks never taken in an interrupt handler.
// Mutex, Semaphore and CondVar sleep on a WaitQueue instead of spinning.
// Every checked lock takes part in the lock-order checker (lockdep_*), which records
// "A was held while B was taken" and reports the first time the opposite order appears.
#pragma once
#include <atomic>
#include "kbase.h"

namespace k {

struct Thread;

class Spinlock {
public:
    constexpr explicit Spinlock(const char* name, bool checked = true)
        : name_(name), checked_(checked)
    {
    }
    Spinlock(const Spinlock&) = delete;
    Spinlock& operator=(const Spinlock&) = delete;

    uint64_t lock_irqsave()
    {
        uint64_t f = irq_save();
        lock();
        return f;
    }
    void unlock_irqrestore(uint64_t f)
    {
        unlock();
        irq_restore(f);
    }
    void lock();     // does not touch the interrupt flag
    void unlock();
    bool held_by_this_cpu() const;
    const char* name() const { return name_; }
    int owner_cpu() const { return owner_cpu_; }
    uint64_t owner_rip() const { return owner_rip_; }

private:
    std::atomic<uint32_t> next_{0};      // next ticket to hand out
    std::atomic<uint32_t> serving_{0};   // ticket now allowed in
    const char* name_;
    bool checked_;
    volatile int owner_cpu_ = -1;
    uint64_t owner_rip_ = 0;
};

// Threads waiting for something. All methods take the scheduler's lock internally.
class WaitQueue {
public:
    // Caller holds 'held' with interrupts disabled. Atomically: enqueue the current
    // thread, release 'held', sleep. Returns with 'held' locked again.
    void sleep(Spinlock& held);
    bool wake_one();
    int wake_all();
    bool empty() const { return head_ == nullptr; }

private:
    friend struct WaitQueueAccess;
    Thread* head_ = nullptr;
    Thread* tail_ = nullptr;
};

class Mutex {
public:
    constexpr explicit Mutex(const char* name) : name_(name), guard_(name, false) {}
    void lock();
    void unlock();
    bool held_by_me() const;
    const char* name() const { return name_; }

private:
    const char* name_;
    Spinlock guard_;            // protects owner_ and waiters_
    Thread* owner_ = nullptr;
    WaitQueue waiters_;
};

class Semaphore {
public:
    constexpr Semaphore(const char* name, int initial) : guard_(name, false), count_(initial) {}
    void down();   // P: wait until count > 0, then decrement
    void up();     // V: increment and wake one waiter

private:
    Spinlock guard_;
    int count_;
    WaitQueue waiters_;
};

class CondVar {
public:
    constexpr explicit CondVar(const char* name) : guard_(name, false) {}
    void wait(Mutex& m);   // release m and sleep atomically; re-acquire m before returning
    void signal();
    void broadcast();

private:
    Spinlock guard_;
    WaitQueue waiters_;
};

// RAII helpers
class MutexGuard {
public:
    explicit MutexGuard(Mutex& m) : m_(m) { m_.lock(); }
    ~MutexGuard() { m_.unlock(); }
    MutexGuard(const MutexGuard&) = delete;
    MutexGuard& operator=(const MutexGuard&) = delete;

private:
    Mutex& m_;
};
class SpinGuard {
public:
    explicit SpinGuard(Spinlock& l) : l_(l), f_(l.lock_irqsave()) {}
    ~SpinGuard() { l_.unlock_irqrestore(f_); }
    SpinGuard(const SpinGuard&) = delete;
    SpinGuard& operator=(const SpinGuard&) = delete;

private:
    Spinlock& l_;
    uint64_t f_;
};

// ---- lock-order checker and lockup detector ----
void lockdep_check_order(const char* name, uint64_t rip);   // before waiting for a lock
void lockdep_mark_held(const char* name);                   // after getting it
void lockdep_release(const char* name);
int lockdep_reports();             // number of problems reported so far
void lockdep_report_bug(const char* what, const char* name, uint64_t rip);
void lockup_set_limit_ms(uint64_t ms, uint64_t tsc_per_ms);
void lockdep_print_held(const Thread* t);

}  // namespace k
