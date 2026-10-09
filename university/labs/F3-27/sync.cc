// sync.cc - ticket spinlock with lockup detection, mutex, semaphore, condition
// variable, and the lock-order checker. Memory orders: acquire on the load that
// lets a CPU in, release on the store that lets the next one in ("C++ Concurrency
// in Action", atomics chapter; Intel SDM Vol. 3 "Memory Ordering"; titles only).
#include "sync.h"
#include "cpu.h"
#include "thread.h"

namespace k {

namespace {
uint64_t lockup_tsc = ~0ull;   // set after the TSC is calibrated
uint64_t lockup_ms = 0;
std::atomic<int> nreports{0};

std::atomic<bool> lockup_reporting{false};

[[noreturn]] void lockup(const Spinlock& l, uint64_t my_rip)
{
    if (lockup_reporting.exchange(true)) {   // another CPU is already reporting: stop here
        for (;;) {
            asm volatile("cli; hlt");
        }
    }
    Cpu& me = this_cpu();
    console_panic_mode();
    kprintf("LOCKUP: cpu%d spun %lu ms on '%s' at rip=%p\n", me.id, lockup_ms, l.name(),
            reinterpret_cast<void*>(my_rip));
    kprintf("  last recorded owner: cpu%d, took it at rip=%p\n", l.owner_cpu(),
            reinterpret_cast<void*>(l.owner_rip()));
    for (int i = 0; i < cpu_count(); ++i) {
        Cpu& c = cpu_by_index(i);
        if (!c.online) {
            continue;
        }
        const Thread* t = c.current;
        kprintf("  cpu%d: thread '%s' (irq depth %d) waiting for '%s', holds: ", c.id,
                t ? t->name : "?", c.irq_depth, c.spinning_on ? c.spinning_on : "-");
        lockdep_print_held(t);
    }
    panic("lockup detected");
}
}  // namespace

void lockup_set_limit_ms(uint64_t ms, uint64_t tsc_per_ms)
{
    lockup_ms = ms;
    lockup_tsc = ms * tsc_per_ms;
}

void Spinlock::lock()
{
    uint64_t rip = reinterpret_cast<uint64_t>(__builtin_return_address(0));
    Cpu& c = this_cpu();
    ++c.preempt_count;
    if (checked_) {
        lockdep_check_order(name_, rip);   // check before we can deadlock
    }
    uint32_t my = next_.fetch_add(1, std::memory_order_relaxed);
    if (serving_.load(std::memory_order_acquire) != my) {
        c.spinning_on = name_;
        uint64_t start = rdtsc();
        while (serving_.load(std::memory_order_acquire) != my) {
            cpu_relax();
            if (rdtsc() - start > lockup_tsc) {
                lockup(*this, rip);
            }
        }
        c.spinning_on = nullptr;
    }
    owner_cpu_ = c.id;
    owner_rip_ = rip;
    if (checked_) {
        lockdep_mark_held(name_);
    }
}

void Spinlock::unlock()
{
    owner_cpu_ = -1;
    serving_.store(serving_.load(std::memory_order_relaxed) + 1, std::memory_order_release);
    if (checked_) {
        lockdep_release(name_);
    }
    --this_cpu().preempt_count;
}

bool Spinlock::held_by_this_cpu() const { return owner_cpu_ == this_cpu().id; }

// ---------------------------------------------------------------- sleeping locks
void Mutex::lock()
{
    uint64_t rip = reinterpret_cast<uint64_t>(__builtin_return_address(0));
    if (this_cpu().irq_depth > 0) {   // a handler must never sleep (B10 acceptance test)
        lockdep_report_bug("mutex acquired in an interrupt handler", name_, rip);
        return;
    }
    lockdep_check_order(name_, rip);
    // Adaptive spinning: while the owner is running on another CPU it will probably
    // release the mutex soon, and spinning is cheaper than sleeping and waking up.
    for (int i = 0; i < 2000; ++i) {
        Thread* o = __atomic_load_n(&owner_, __ATOMIC_RELAXED);
        if (o == nullptr || o->state != ThreadState::Running || o == this_cpu().current) {
            break;
        }
        cpu_relax();
    }
    uint64_t f = guard_.lock_irqsave();
    while (owner_ != nullptr) {
        waiters_.sleep(guard_);
    }
    owner_ = this_cpu().current;
    guard_.unlock_irqrestore(f);
    lockdep_mark_held(name_);
}

void Mutex::unlock()
{
    uint64_t f = guard_.lock_irqsave();
    owner_ = nullptr;
    waiters_.wake_one();
    guard_.unlock_irqrestore(f);
    lockdep_release(name_);
}

bool Mutex::held_by_me() const { return owner_ == this_cpu().current; }

void Semaphore::down()
{
    uint64_t f = guard_.lock_irqsave();
    while (count_ == 0) {
        waiters_.sleep(guard_);
    }
    --count_;
    guard_.unlock_irqrestore(f);
}

void Semaphore::up()
{
    uint64_t f = guard_.lock_irqsave();
    ++count_;
    waiters_.wake_one();
    guard_.unlock_irqrestore(f);
}

void CondVar::wait(Mutex& m)
{
    uint64_t f = guard_.lock_irqsave();
    m.unlock();              // a signaller now needs guard_, which we hold until we sleep
    waiters_.sleep(guard_);
    guard_.unlock_irqrestore(f);
    m.lock();
}

void CondVar::signal()
{
    uint64_t f = guard_.lock_irqsave();
    waiters_.wake_one();
    guard_.unlock_irqrestore(f);
}

void CondVar::broadcast()
{
    uint64_t f = guard_.lock_irqsave();
    waiters_.wake_all();
    guard_.unlock_irqrestore(f);
}

// ---------------------------------------------------------------- lock-order checker
namespace {
constexpr int kClasses = 64;
const char* class_name[kClasses];
int nclasses = 0;
bool before[kClasses][kClasses];        // before[a][b]: b was taken while a was held
uint64_t before_rip[kClasses][kClasses];
bool reported[kClasses][kClasses];
std::atomic<bool> table_busy{false};    // tiny raw lock for the tables

void table_lock()
{
    while (table_busy.exchange(true, std::memory_order_acquire)) {
        cpu_relax();
    }
}
void table_unlock() { table_busy.store(false, std::memory_order_release); }

int class_of(const char* name)   // locks with the same name share one class
{
    for (int i = 0; i < nclasses; ++i) {
        if (class_name[i] == name || str_eq(class_name[i], name)) {
            return i;
        }
    }
    if (nclasses == kClasses) {
        return -1;
    }
    class_name[nclasses] = name;
    return nclasses++;
}
}  // namespace

void lockdep_check_order(const char* name, uint64_t rip)
{
    Thread* t = this_cpu().current;
    if (t == nullptr) {
        return;
    }
    uint64_t f = irq_save();
    table_lock();
    int b = class_of(name);
    for (int i = 0; i < t->nheld && b >= 0; ++i) {
        int a = t->held[i];
        if (a == b) {
            continue;
        }
        if (before[b][a] && !reported[a][b]) {
            reported[a][b] = reported[b][a] = true;
            nreports.fetch_add(1);
            kprintf("LOCKDEP: lock order inversion on cpu%d, thread '%s'\n"
                    "  now:     acquiring '%s' while holding '%s' (at %p)\n"
                    "  earlier: '%s' was held while acquiring '%s' (at %p)\n",
                    this_cpu().id, t->name, class_name[b], class_name[a],
                    reinterpret_cast<void*>(rip), class_name[b], class_name[a],
                    reinterpret_cast<void*>(before_rip[b][a]));
        }
        if (!before[a][b]) {
            before[a][b] = true;
            before_rip[a][b] = rip;
        }
    }
    table_unlock();
    irq_restore(f);
}

void lockdep_mark_held(const char* name)
{
    Thread* t = this_cpu().current;
    if (t == nullptr) {
        return;
    }
    uint64_t f = irq_save();
    table_lock();
    int b = class_of(name);
    if (b >= 0 && t->nheld < kMaxHeld) {
        t->held[t->nheld++] = b;
    }
    table_unlock();
    irq_restore(f);
}

void lockdep_release(const char* name)
{
    Thread* t = this_cpu().current;
    if (t == nullptr) {
        return;
    }
    uint64_t f = irq_save();
    table_lock();
    int b = class_of(name);
    for (int i = t->nheld - 1; i >= 0; --i) {   // usually the last one (LIFO)
        if (t->held[i] == b) {
            for (int j = i; j + 1 < t->nheld; ++j) {
                t->held[j] = t->held[j + 1];
            }
            --t->nheld;
            break;
        }
    }
    table_unlock();
    irq_restore(f);
}

void lockdep_print_held(const Thread* t)
{
    if (t == nullptr || t->nheld == 0) {
        kprintf("(none)\n");
        return;
    }
    for (int i = 0; i < t->nheld; ++i) {
        kprintf("'%s'%s", class_name[t->held[i]], i + 1 < t->nheld ? ", " : "\n");
    }
}

void lockdep_report_bug(const char* what, const char* name, uint64_t rip)
{
    nreports.fetch_add(1);
    kprintf("LOCKDEP: BUG: %s: '%s' at %p (cpu%d, thread '%s')\n", what, name,
            reinterpret_cast<void*>(rip), this_cpu().id,
            this_cpu().current ? this_cpu().current->name : "?");
}

int lockdep_reports() { return nreports.load(); }

}  // namespace k
