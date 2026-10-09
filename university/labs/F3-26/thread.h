// thread.h - kernel threads and the scheduler of the OS303 kernel (milestone B9).
#pragma once
#include "cpu.h"
#include "sync.h"

namespace k {

struct Process;

enum class ThreadState : uint8_t { Ready, Running, Blocked, Dead };

struct Thread {
    uint64_t rsp;               // saved stack pointer while not running (switch.S uses offset 0)
    uint64_t kstack_top;        // top of this thread's kernel stack
    int id;
    char name[16];
    ThreadState state;
    Thread* next;               // link in the run queue, a wait queue or the sleep list
    Thread* all_next;           // link in the list of all threads (for the watchdog dump)
    int affinity;               // -1: any CPU; otherwise the only CPU allowed to run it
    bool kill_requested;
    uint64_t wake_tick;         // for thread_sleep
    int stack_slot;             // which slot of the stack area (with its guard page)
    void (*entry)(void*);
    void* arg;
    Process* proc;              // nullptr for kernel threads
    int held[kMaxHeld];         // lock classes held (lock-order checker)
    int nheld;
    uint64_t runs;              // how many times it was switched in
    int last_cpu;
};

constexpr uint64_t kStackPages = 4;       // 16 KiB kernel stack per thread
constexpr int kTickHz = 100;              // scheduler tick: 10 ms quantum
constexpr int kQuantumTicks = 1;

void sched_init_cpu(Cpu& c, const char* idle_name);    // make the running code a thread
[[noreturn]] void idle_forever();
Thread* thread_create(const char* name, void (*fn)(void*), void* arg, int affinity = -1,
                      Process* proc = nullptr);
[[noreturn]] void thread_exit();
void thread_yield();
void thread_sleep_ticks(uint64_t ticks);
void thread_kill(Thread* t);           // takes effect at its next preemption
void thread_reap_zombies();            // free stacks of dead threads (needs interrupts on)
int threads_live();
void sched_dump();                     // print every thread and every CPU (watchdog)
int stack_slots_in_use();
uint64_t global_ticks();

// Scheduler internals used by WaitQueue, the timer and the process code.
void schedule();                        // call with interrupts disabled
void sched_timer_tick();                // from the timer interrupt
void sched_make_ready(Thread* t);       // caller holds the scheduler lock
Spinlock& sched_lock();
extern "C" void finish_switch();
extern "C" void switch_context(uint64_t* save_rsp, uint64_t load_rsp);   // switch.S

}  // namespace k
