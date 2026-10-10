// test_threads.cc - the B9 acceptance tests, and the F3-26 forensic case.
#include "hooks.h"
#include "thread.h"

namespace k {

namespace {
constexpr int kThreads = 100;
std::atomic<uint64_t> counters[kThreads];
std::atomic<bool> stop{false};
std::atomic<int> finished{0};

void count_forever(void* arg)   // never yields: only preemption lets others run
{
    auto i = reinterpret_cast<uintptr_t>(arg);
    while (!stop.load(std::memory_order_relaxed)) {
        counters[i].store(counters[i].load(std::memory_order_relaxed) + 1,
                          std::memory_order_relaxed);
    }
    finished.fetch_add(1);
}

void spin_forever(void*)
{
    for (;;) {
        asm volatile("pause");   // an infinite loop with no yield and no lock
    }
}

void quick(void*) { finished.fetch_add(1); }

void wait_finished(int n)
{
    while (finished.load() < n) {
        thread_yield();
    }
}

struct Stats {
    uint64_t frames, objects;
    int slots, threads;
};
Stats snapshot()
{
    thread_reap_zombies();
    return Stats{frames_free(), heap_live_objects(), stack_slots_in_use(), threads_live()};
}
void print(const char* label, const Stats& s)
{
    kprintf("%s: free frames %lu, heap objects %lu, stack slots %d, live threads %d\n", label,
            s.frames, s.objects, s.slots, s.threads);
}
}  // namespace

bool test_threads()
{
    bool ok = true;

    // 1. Fairness: 100 threads that never yield, 5 s of scheduler ticks.
    const uint64_t seconds = 5;
    for (int i = 0; i < kThreads; ++i) {
        char name[16] = "count";
        name[5] = char('0' + i / 10);
        name[6] = char('0' + i % 10);
        thread_create(name, count_forever, reinterpret_cast<void*>(uintptr_t(i)));
    }
    uint64_t t0 = global_ticks();
    thread_sleep_ticks(seconds * kTickHz);
    stop = true;
    wait_finished(kThreads);
    uint64_t lo = ~0ull, hi = 0, sum = 0;
    for (auto& c : counters) {
        uint64_t v = c.load();
        lo = v < lo ? v : lo;
        hi = v > hi ? v : hi;
        sum += v;
    }
    kprintf("fairness: %d threads, %lu ticks of %d ms; counters min %lu, max %lu, mean %lu\n",
            kThreads, global_ticks() - t0, 1000 / kTickHz, lo, hi, sum / kThreads);
    kprintf("fairness: max/min = %lu.%02lu (bound: all non-zero and max/min <= 2.00)\n",
            lo ? hi / lo : 0, lo ? (hi * 100 / lo) % 100 : 0);
    ok = ok && lo > 0 && hi <= 2 * lo;

    // 2. Preemption: a thread in an infinite loop does not stop the others.
    finished = 0;
    Thread* spinner = thread_create("spinner", spin_forever, nullptr);
    for (int i = 0; i < 10; ++i) {
        thread_create("quick", quick, nullptr);
    }
    for (int i = 0; i < 5; ++i) {
        thread_sleep_ticks(20);   // main can only wake up if the timer preempts the spinner
        kprintf("preemption: main woke at tick %lu while 'spinner' loops (switched in %lu "
                "times so far)\n", global_ticks(), spinner->runs);
    }
    wait_finished(10);
    kprintf("preemption: the 10 'quick' threads finished too\n");
    thread_kill(spinner);   // dies at its next preemption point
    while (threads_live() > 1) {   // only idle0 is left (main is not counted)
        thread_yield();
        thread_reap_zombies();
    }
    kprintf("preemption: spinner killed at a preemption point\n");

    // 3. 100,000 create/exit cycles: stacks, frames and heap objects return to baseline.
    finished = 0;
    for (int i = 0; i < 100; ++i) {   // warm-up: page tables of the stack area exist now
        thread_create("warm", quick, nullptr);
    }
    wait_finished(100);
    Stats base = snapshot();
    print("baseline", base);
    const int rounds = 1000, per_round = 100;
    for (int r = 0; r < rounds; ++r) {
        finished = 0;
        for (int i = 0; i < per_round; ++i) {
            thread_create("cycle", quick, nullptr);
        }
        wait_finished(per_round);
    }
    Stats after = snapshot();
    print("after 100000 create/exit", after);
    bool same = after.frames == base.frames && after.objects == base.objects &&
                after.slots == base.slots && after.threads == base.threads;
    kprintf("create/exit: %s\n", same ? "no leaked stacks, frames or heap objects" : "LEAK");
    return ok && same;
}

// ------------------------------------------------------------ forensic case (F3-26)
namespace {
// A recursive-descent parser for nested parentheses, as a colleague wrote it.
int parse_group(const char* s, int depth)
{
    volatile char scratch[1024];   // a "small" buffer per level
    scratch[0] = s[0];
    if (s[0] == '(') {
        return 1 + parse_group(s + 1, depth + 1) + scratch[0] - '(';
    }
    return 0;
}

void parser_thread(void* arg)
{
    const char* input = static_cast<const char*>(arg);
    kprintf("parser: depth %d\n", parse_group(input, 0));
    finished.fetch_add(1);
}
}  // namespace

bool test_stack_overflow()
{
    static char shallow[8], deep[40];
    for (int i = 0; i < 7; ++i) {
        shallow[i] = '(';
    }
    for (int i = 0; i < 39; ++i) {
        deep[i] = '(';
    }
    finished = 0;
    thread_create("parser", parser_thread, shallow);
    wait_finished(1);
    thread_create("parser", parser_thread, deep);   // the crash happens here
    wait_finished(2);
    return true;
}

}  // namespace k
