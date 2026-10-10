// test_sync.cc - the B10 acceptance tests and the F3-27 forensic case.
#include "hooks.h"
#include "thread.h"

namespace k {

// ------------------------------------------------------------ bounded buffer
namespace {
constexpr int kSlots = 64;
constexpr uint64_t kMaxItems = 10'000'000;
uint64_t seen[kMaxItems / 64];        // one bit per item: catches loss and duplicates

struct Buffer {
    Mutex m{"buffer"};
    CondVar not_full{"not_full"};
    CondVar not_empty{"not_empty"};
    uint64_t slot[kSlots];
    int head = 0, count = 0;
};
Buffer buf;

struct Run {
    uint64_t items;
    int producers, consumers;
    std::atomic<uint64_t> consumed{0};
    std::atomic<uint64_t> duplicates{0};
    std::atomic<int> producers_finished{0};
    std::atomic<int> done{0};
};
Run* run;

void put(uint64_t v)
{
    MutexGuard g(buf.m);
    while (buf.count == kSlots) {
        buf.not_full.wait(buf.m);
    }
    buf.slot[(buf.head + buf.count) % kSlots] = v;
    ++buf.count;
    buf.not_empty.signal();
}

uint64_t get()
{
    MutexGuard g(buf.m);
    while (buf.count == 0) {
        buf.not_empty.wait(buf.m);
    }
    uint64_t v = buf.slot[buf.head];
    buf.head = (buf.head + 1) % kSlots;
    --buf.count;
    buf.not_full.signal();
    return v;
}

void producer(void* arg)   // producer p makes items p, p + P, p + 2P, ...
{
    auto p = reinterpret_cast<uint64_t>(arg);
    for (uint64_t v = p; v < run->items; v += uint64_t(run->producers)) {
        put(v);
    }
    if (run->producers_finished.fetch_add(1) + 1 == run->producers) {
        for (int i = 0; i < run->consumers; ++i) {
            put(~0ull);   // the last producer puts one "stop" marker per consumer
        }
    }
    run->done.fetch_add(1);
}

void consumer(void*)
{
    for (;;) {
        uint64_t v = get();
        if (v == ~0ull) {
            break;
        }
        uint64_t bit = 1ull << (v % 64);
        if (__atomic_fetch_or(&seen[v / 64], bit, __ATOMIC_RELAXED) & bit) {
            run->duplicates.fetch_add(1);
        }
        run->consumed.fetch_add(1, std::memory_order_relaxed);
    }
    run->done.fetch_add(1);
}
}  // namespace

// Used by this test on one CPU and again by test_smp on many CPUs.
bool producer_consumer(uint64_t items, int producers, int consumers)
{
    memset(seen, 0, sizeof(seen));
    Run r;
    r.items = items;
    r.producers = producers;
    r.consumers = consumers;
    run = &r;
    uint64_t t0 = global_ticks();
    for (int i = 0; i < producers; ++i) {
        thread_create("producer", producer, reinterpret_cast<void*>(uint64_t(i)));
    }
    for (int i = 0; i < consumers; ++i) {
        thread_create("consumer", consumer, nullptr);
    }
    while (r.done.load() < producers + consumers) {
        thread_sleep_ticks(1);
        if (global_ticks() - t0 == 3000) {   // deadlock watchdog: 30 s without finishing
            kprintf("watchdog: consumed %lu of %lu, buffer count %d\n", r.consumed.load(),
                    items, buf.count);
            sched_dump();
        }
    }
    uint64_t missing = 0;
    for (uint64_t v = 0; v < items; ++v) {
        missing += ((seen[v / 64] >> (v % 64)) & 1) == 0;
    }
    kprintf("producer/consumer: %lu items, %d producers, %d consumers, %d CPUs: consumed %lu, "
            "missing %lu, duplicates %lu (%lu ticks)\n", items, producers, consumers,
            cpu_count(), r.consumed.load(), missing, r.duplicates.load(), global_ticks() - t0);
    return r.consumed.load() == items && missing == 0 && r.duplicates.load() == 0;
}

// ------------------------------------------------------------ semaphores
namespace {
Semaphore sem_items{"sem_items", 0};
Semaphore sem_space{"sem_space", kSlots};
Spinlock ring_lock{"ring"};
uint64_t ring[kSlots];
int ring_in = 0, ring_out = 0;
std::atomic<uint64_t> sem_sum{0};
std::atomic<int> sem_done{0};
constexpr uint64_t kSemItems = 1'000'000;

void sem_producer(void*)
{
    for (uint64_t v = 1; v <= kSemItems; ++v) {
        sem_space.down();
        uint64_t f = ring_lock.lock_irqsave();
        ring[ring_in] = v;
        ring_in = (ring_in + 1) % kSlots;
        ring_lock.unlock_irqrestore(f);
        sem_items.up();
    }
    sem_done.fetch_add(1);
}

void sem_consumer(void*)
{
    for (uint64_t i = 0; i < kSemItems; ++i) {
        sem_items.down();
        uint64_t f = ring_lock.lock_irqsave();
        uint64_t v = ring[ring_out];
        ring_out = (ring_out + 1) % kSlots;
        ring_lock.unlock_irqrestore(f);
        sem_space.up();
        sem_sum.fetch_add(v, std::memory_order_relaxed);
    }
    sem_done.fetch_add(1);
}

// ------------------------------------------------------------ checker tests
Mutex lock_a{"A"}, lock_b{"B"};
Mutex irq_victim{"irq_victim"};
Spinlock counter_lock{"counter"};
uint64_t shared_counter = 0;
std::atomic<uint64_t> hook_increments{0};

void mutex_in_irq_hook()
{
    timer_hook = nullptr;   // one shot
    irq_victim.lock();      // a bug: must be detected, not executed
}

void counter_hook()         // timer interrupt also updates the counter
{
    counter_lock.lock();    // interrupts are already off inside a handler
    ++shared_counter;
    counter_lock.unlock();
    hook_increments.fetch_add(1);
}
}  // namespace

bool test_sync()
{
    bool ok = producer_consumer(kMaxItems, 1, 1);

    uint64_t t0 = global_ticks();
    thread_create("sem_producer", sem_producer, nullptr);
    thread_create("sem_consumer", sem_consumer, nullptr);
    while (sem_done.load() < 2) {
        thread_sleep_ticks(1);
    }
    uint64_t expect = kSemItems * (kSemItems + 1) / 2;
    kprintf("semaphores: %lu items, sum %lu (expected %lu) (%lu ticks)\n", kSemItems,
            sem_sum.load(), expect, global_ticks() - t0);
    ok = ok && sem_sum.load() == expect;

    int before = lockdep_reports();
    lock_a.lock();
    lock_b.lock();   // records: A is held while B is taken
    lock_b.unlock();
    lock_a.unlock();
    lock_b.lock();
    lock_a.lock();   // the opposite order: must be reported (it cannot deadlock here, one thread)
    lock_a.unlock();
    lock_b.unlock();
    bool order_reported = lockdep_reports() == before + 1;
    kprintf("lock-order checker: inverted pair %s\n", order_reported ? "reported" : "NOT reported");

    before = lockdep_reports();
    timer_hook = mutex_in_irq_hook;
    thread_sleep_ticks(3);
    bool irq_reported = lockdep_reports() == before + 1;
    kprintf("mutex in interrupt handler: %s\n", irq_reported ? "reported" : "NOT reported");

    // A spinlock shared with an interrupt handler, taken correctly with lock_irqsave().
    timer_hook = counter_hook;
    uint64_t mine = 0;
    for (uint64_t start = global_ticks(); global_ticks() - start < 100;) {
        uint64_t f = counter_lock.lock_irqsave();
        ++shared_counter;
        counter_lock.unlock_irqrestore(f);
        ++mine;
    }
    timer_hook = nullptr;
    thread_sleep_ticks(2);
    kprintf("irq-shared counter: %lu = %lu (thread) + %lu (handler)\n", shared_counter, mine,
            hook_increments.load());
    ok = ok && shared_counter == mine + hook_increments.load();
    return ok && order_reported && irq_reported;
}

// ------------------------------------------------------------ forensic case (F3-27)
// A colleague's "faster" statistics counter: plain lock() without disabling
// interrupts, because "the critical section is tiny".
namespace {
Spinlock stats_lock{"stats"};
uint64_t stats_events = 0;

void stats_hook()
{
    stats_lock.lock();
    ++stats_events;
    stats_lock.unlock();
}

void stats_worker(void*)
{
    for (;;) {
        stats_lock.lock();   // interrupts stay enabled while the lock is held
        for (int i = 0; i < 2000; ++i) {
            stats_events = stats_events + 1;
            asm volatile("" ::: "memory");
        }
        stats_lock.unlock();
    }
}
}  // namespace

bool test_irq_lock()
{
    timer_hook = stats_hook;
    thread_create("stats_worker", stats_worker, nullptr);
    for (int s = 1; s <= 10; ++s) {
        thread_sleep_ticks(100);
        kprintf("still alive after %d s, %lu events\n", s, stats_events);
    }
    timer_hook = nullptr;
    return true;
}

}  // namespace k
