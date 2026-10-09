// thread.cc - kernel threads, run queue, preemptive round-robin scheduler (B9).
// One global run queue protected by one spinlock (curriculum B11: "a global queue
// first"). The scheduler lock is taken by the thread that switches away and released
// by the thread that switches in (finish_switch), so no other CPU can pick up a thread
// whose registers are still being saved.
#include "thread.h"
#include "hooks.h"

extern "C" char thread_start[], idle_entry[];   // switch.S

namespace k {

namespace {
Spinlock g_sched("sched", false);   // not in the lock-order checker: it is handed over
Thread* rq_head = nullptr;          // run queue: FIFO of Ready threads
Thread* rq_tail = nullptr;
Thread* sleepers = nullptr;         // Blocked in thread_sleep_ticks
Thread* zombies = nullptr;          // Dead, stack not yet freed
Thread* all_threads = nullptr;      // every thread not yet freed
std::atomic<int> next_id{0};
std::atomic<int> nlive{0};
std::atomic<uint64_t> g_ticks{0};   // ticks of CPU 0's timer since boot
Thread boot_thread;                 // kmain becomes this thread

constexpr int kMaxSlots = 8192;
constexpr uint64_t kSlotPages = kStackPages + 1;   // one guard page below each stack
bool slot_used[kMaxSlots];
int nslots = 0;
Spinlock slot_lock("stack-slots");

void rq_push(Thread* t)
{
    t->next = nullptr;
    if (rq_tail) {
        rq_tail->next = t;
    } else {
        rq_head = t;
    }
    rq_tail = t;
}

Thread* rq_pop_for(int cpu)   // first Ready thread allowed on this CPU
{
    Thread* prev = nullptr;
    for (Thread* t = rq_head; t != nullptr; prev = t, t = t->next) {
        if (t->affinity < 0 || t->affinity == cpu) {
            (prev ? prev->next : rq_head) = t->next;
            if (rq_tail == t) {
                rq_tail = prev;
            }
            t->next = nullptr;
            return t;
        }
    }
    return nullptr;
}

int slot_alloc()
{
    uint64_t f = slot_lock.lock_irqsave();
    int s = -1;
    for (int i = 0; i < kMaxSlots; ++i) {
        if (!slot_used[i]) {
            slot_used[i] = true;
            ++nslots;
            s = i;
            break;
        }
    }
    slot_lock.unlock_irqrestore(f);
    return s;
}

uint64_t slot_base(int s) { return kStackArea + uint64_t(s) * kSlotPages * kPage; }

Thread* thread_alloc(const char* name, void (*fn)(void*), void* arg, int affinity)
{
    auto* t = static_cast<Thread*>(kmalloc(sizeof(Thread)));
    t->id = next_id.fetch_add(1);
    str_copy(t->name, name, sizeof(t->name));
    t->affinity = affinity;
    t->entry = fn;
    t->arg = arg;
    t->stack_slot = slot_alloc();
    if (t->stack_slot < 0) {
        panic("out of kernel stack slots");
    }
    uint64_t base = slot_base(t->stack_slot);   // base itself is the guard page: never mapped
    for (uint64_t p = 1; p < kSlotPages; ++p) {
        kmap_page(base + p * kPage, frame_alloc());
    }
    t->kstack_top = base + kSlotPages * kPage;
    // The first switch_context into this thread pops six registers and returns into
    // thread_start, which calls fn(arg) with fn in r12 and arg in r13.
    auto* sp = reinterpret_cast<uint64_t*>(t->kstack_top);
    *--sp = reinterpret_cast<uint64_t>(thread_start);   // return address
    *--sp = 0;                                           // rbp
    *--sp = 0;                                           // rbx
    *--sp = reinterpret_cast<uint64_t>(fn);              // r12
    *--sp = reinterpret_cast<uint64_t>(arg);             // r13
    *--sp = 0;                                           // r14
    *--sp = 0;                                           // r15
    t->rsp = reinterpret_cast<uint64_t>(sp);
    t->state = ThreadState::Ready;
    nlive.fetch_add(1);
    uint64_t f = g_sched.lock_irqsave();
    t->all_next = all_threads;
    all_threads = t;
    g_sched.unlock_irqrestore(f);
    return t;
}

// Called with the scheduler lock held; returns with it released.
void schedule_locked()
{
    Cpu& c = this_cpu();
    Thread* prev = c.current;
    c.need_resched = false;
    if (prev->state == ThreadState::Running) {
        if (prev->kill_requested && prev->proc == nullptr) {
            prev->state = ThreadState::Dead;   // a kernel thread killed at a preemption point
        } else if (prev != c.idle) {
            prev->state = ThreadState::Ready;
            rq_push(prev);
        }
    }
    if (prev->state == ThreadState::Dead) {
        prev->next = zombies;   // freed later by thread_reap_zombies(), never on its own stack
        zombies = prev;
    }
    Thread* next = rq_pop_for(c.id);
    if (next == nullptr) {
        next = (prev->state == ThreadState::Running) ? prev : c.idle;
    }
    if (next == prev) {
        prev->state = ThreadState::Running;
        g_sched.unlock();
        return;
    }
    next->state = ThreadState::Running;
    next->runs++;
    next->last_cpu = c.id;
    c.current = next;
    c.slice = 0;
    c.kernel_rsp = next->kstack_top;   // used by the SYSCALL entry (F3-29)
    c.tss.rsp0 = next->kstack_top;     // used by interrupts from ring 3 (F3-29)
    uint64_t cr3 = next->proc ? proc_cr3(next->proc) : kernel_cr3();
    if (read_cr3() != cr3) {
        write_cr3(cr3);
    }
    switch_context(&prev->rsp, next->rsp);
    finish_switch();   // we are prev again, possibly on another CPU
}
}  // namespace

extern "C" void finish_switch() { g_sched.unlock(); }
extern "C" [[noreturn]] void thread_exit_from_start() { thread_exit(); }

Spinlock& sched_lock() { return g_sched; }
uint64_t global_ticks() { return g_ticks.load(); }
int threads_live() { return nlive.load(); }
int stack_slots_in_use() { return nslots; }

void sched_init_cpu(Cpu& c, const char* idle_name)
{
    if (c.id == 0) {   // the boot code becomes thread "main"; a fresh idle thread is made
        Thread* t = &boot_thread;
        t->id = next_id.fetch_add(1);
        str_copy(t->name, "main", sizeof(t->name));
        t->state = ThreadState::Running;
        t->affinity = -1;
        t->stack_slot = -1;
        c.current = t;
        c.idle = thread_alloc(idle_name, nullptr, nullptr, 0);
        reinterpret_cast<uint64_t*>(c.idle->rsp)[6] = reinterpret_cast<uint64_t>(idle_entry);
    } else {           // an application processor: its start-up code becomes its idle thread
        auto* t = static_cast<Thread*>(kmalloc(sizeof(Thread)));
        t->id = next_id.fetch_add(1);
        str_copy(t->name, idle_name, sizeof(t->name));
        t->state = ThreadState::Running;
        t->affinity = c.id;
        t->stack_slot = -1;
        nlive.fetch_add(1);
        c.current = c.idle = t;
    }
}

void idle_forever()
{
    for (;;) {
        irq_disable();
        schedule();                               // run anything that is ready
        asm volatile("sti; hlt" ::: "memory");   // then sleep until the next interrupt
    }
}

extern "C" [[noreturn]] void idle_loop()   // first code of CPU 0's idle thread
{
    finish_switch();
    idle_forever();
}

Thread* thread_create(const char* name, void (*fn)(void*), void* arg, int affinity, Process* proc)
{
    thread_reap_zombies();
    Thread* t = thread_alloc(name, fn, arg, affinity);
    t->proc = proc;   // set before the thread can run: schedule() loads its CR3
    uint64_t f = g_sched.lock_irqsave();
    sched_make_ready(t);
    g_sched.unlock_irqrestore(f);
    return t;
}

void sched_make_ready(Thread* t)
{
    t->state = ThreadState::Ready;
    rq_push(t);
    Cpu& me = this_cpu();
    for (int i = 0; i < cpu_count(); ++i) {   // wake one idle CPU that may run it
        Cpu& c = cpu_by_index(i);
        if (&c != &me && c.online && c.current == c.idle && (t->affinity < 0 || t->affinity == i)) {
            lapic_send_ipi(c.apic_id, kVecResched);
            break;
        }
    }
}

void schedule()
{
    g_sched.lock();
    schedule_locked();
}

void thread_yield()
{
    uint64_t f = irq_save();
    schedule();
    irq_restore(f);
}

void thread_exit()
{
    irq_disable();
    this_cpu().current->state = ThreadState::Dead;
    schedule();
    panic("a dead thread was scheduled");
}

void thread_sleep_ticks(uint64_t ticks)
{
    uint64_t f = irq_save();
    g_sched.lock();
    Thread* t = this_cpu().current;
    t->wake_tick = g_ticks.load() + ticks;
    t->state = ThreadState::Blocked;
    t->next = sleepers;
    sleepers = t;
    schedule_locked();
    irq_restore(f);
}

void thread_kill(Thread* t) { t->kill_requested = true; }

void sched_dump()
{
    static const char* const names[] = {"ready", "running", "blocked", "dead"};
    kprintf("scheduler dump at tick %lu:\n", g_ticks.load());
    for (int i = 0; i < cpu_count(); ++i) {
        Cpu& c = cpu_by_index(i);
        kprintf("  cpu%d: running '%s' (id %d)\n", c.id, c.current ? c.current->name : "?",
                c.current ? c.current->id : -1);
    }
    for (Thread* t = all_threads; t != nullptr; t = t->all_next) {
        kprintf("  thread %d '%s': %s, last cpu %d, switched in %lu times, holds: ", t->id,
                t->name, names[int(t->state)], t->last_cpu, t->runs);
        lockdep_print_held(t);
    }
}

void sched_timer_tick()   // timer interrupt, interrupts disabled
{
    Cpu& c = this_cpu();
    c.ticks = c.ticks + 1;
    if (c.id == 0) {
        g_ticks.fetch_add(1);
        g_sched.lock();
        for (Thread** pp = &sleepers; *pp != nullptr;) {
            Thread* t = *pp;
            if (t->wake_tick <= g_ticks.load()) {
                *pp = t->next;
                sched_make_ready(t);
            } else {
                pp = &t->next;
            }
        }
        g_sched.unlock();
    }
    if (++c.slice >= kQuantumTicks || c.current == c.idle) {
        c.need_resched = true;   // acted on when the interrupt handler finishes
    }
}

void thread_reap_zombies()
{
    if (!irqs_enabled()) {
        return;   // a TLB shootdown waits for other CPUs: never with interrupts off
    }
    uint64_t f = g_sched.lock_irqsave();
    Thread* list = zombies;
    zombies = nullptr;
    g_sched.unlock_irqrestore(f);
    while (list != nullptr) {
        Thread* t = list;
        list = t->next;
        if (t->stack_slot >= 0) {
            uint64_t base = slot_base(t->stack_slot);
            for (uint64_t p = 1; p < kSlotPages; ++p) {
                frame_free(kunmap_page(base + p * kPage));
            }
            tlb_shootdown(base, kSlotPages);   // no CPU may keep a stale translation
            uint64_t g = slot_lock.lock_irqsave();
            slot_used[t->stack_slot] = false;
            --nslots;
            slot_lock.unlock_irqrestore(g);
        }
        uint64_t h = g_sched.lock_irqsave();
        for (Thread** pp = &all_threads; *pp != nullptr; pp = &(*pp)->all_next) {
            if (*pp == t) {
                *pp = t->all_next;
                break;
            }
        }
        g_sched.unlock_irqrestore(h);
        kfree(t);
        nlive.fetch_sub(1);
    }
}

// ---------------------------------------------------------------- wait queues
void WaitQueue::sleep(Spinlock& held)
{
    g_sched.lock();
    Thread* t = this_cpu().current;
    t->state = ThreadState::Blocked;
    t->next = nullptr;
    if (tail_) {
        tail_->next = t;
    } else {
        head_ = t;
    }
    tail_ = t;
    held.unlock();      // interrupts stay disabled: no wake-up can be lost in between
    schedule_locked();
    held.lock();
}

bool WaitQueue::wake_one()
{
    uint64_t f = g_sched.lock_irqsave();
    Thread* t = head_;
    if (t) {
        head_ = t->next;
        if (head_ == nullptr) {
            tail_ = nullptr;
        }
        sched_make_ready(t);
    }
    g_sched.unlock_irqrestore(f);
    return t != nullptr;
}

int WaitQueue::wake_all()
{
    int n = 0;
    while (wake_one()) {
        ++n;
    }
    return n;
}

}  // namespace k
