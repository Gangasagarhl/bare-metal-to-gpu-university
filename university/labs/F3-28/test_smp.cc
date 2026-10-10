// test_smp.cc - the B11 acceptance tests and the F3-28 forensic case.
#include "hooks.h"
#include "thread.h"

extern "C" bool probe_read(const uint64_t* addr, uint64_t* out);   // uaccess.S

namespace k {

bool producer_consumer(uint64_t items, int producers, int consumers);   // test_sync.cc
int smp_madt_cpus();
uint64_t tlb_shootdown_count();
extern bool tlb_shootdown_enabled;

namespace {
uint64_t arg_number(const char* key, uint64_t fallback)
{
    const char* v = boot_arg(key);
    if (v == nullptr) {
        return fallback;
    }
    uint64_t n = 0;
    while (*v >= '0' && *v <= '9') {
        n = n * 10 + uint64_t(*v++ - '0');
    }
    return n;
}

std::atomic<int> done{0};
std::atomic<uint32_t> cpus_used{0};

void where_am_i(void*)
{
    for (int i = 0; i < 20; ++i) {
        cpus_used.fetch_or(1u << this_cpu().id);
        thread_sleep_ticks(1);
    }
    done.fetch_add(1);
}

// TLB shootdown test: one prober per CPU reads the probe page until the read faults.
struct Probe {
    std::atomic<uint64_t> reads{0};    // successful reads
    std::atomic<uint64_t> stale{0};    // successful reads that started after the shootdown
    std::atomic<bool> faulted{false};
    uint64_t reads_at_unmap = 0;
};
Probe probes[kMaxCpus];
std::atomic<bool> stop_probing{false};
std::atomic<bool> shootdown_done{false};

void prober(void* arg)
{
    Probe& p = probes[reinterpret_cast<uintptr_t>(arg)];
    uint64_t v;
    while (!stop_probing.load()) {
        bool late = shootdown_done.load(std::memory_order_acquire);
        if (!probe_read(reinterpret_cast<const uint64_t*>(kProbeArea), &v)) {
            p.faulted = true;   // the page fault was turned into "false" by the fixup table
            break;
        }
        p.reads.fetch_add(1, std::memory_order_relaxed);
        if (late) {
            p.stale.fetch_add(1, std::memory_order_relaxed);   // read through a stale TLB entry
        }
    }
    done.fetch_add(1);
}

bool shootdown_test()
{
    int n = cpu_count();
    uint64_t frame = frame_alloc();
    *reinterpret_cast<uint64_t*>(frame) = 0x5eed;
    kmap_page(kProbeArea, frame);
    done = 0;
    for (int i = 0; i < n; ++i) {
        thread_create("prober", prober, reinterpret_cast<void*>(uintptr_t(i)), i);   // pinned
    }
    for (int i = 0; i < n; ++i) {
        while (probes[i].reads.load() == 0) {   // every CPU has used the mapping
            thread_yield();
        }
    }
    for (int i = 0; i < n; ++i) {
        probes[i].reads_at_unmap = probes[i].reads.load();
    }
    frame_free(kunmap_page(kProbeArea));     // the page-table entry is gone ...
    tlb_shootdown(kProbeArea, 1);             // ... and every CPU is told to forget it
    shootdown_done.store(true, std::memory_order_release);
    thread_sleep_ticks(50);
    stop_probing = true;
    while (done.load() < n) {
        thread_yield();
    }
    bool ok = true;
    for (int i = 0; i < n; ++i) {
        Probe& p = probes[i];
        uint64_t stale = p.stale.load();
        kprintf("shootdown: cpu%d read the page %lu times before the unmap, %lu after the "
                "shootdown completed; %s\n", i, p.reads_at_unmap, stale,
                p.faulted ? "then faulted" : "NEVER FAULTED (stale TLB entry)");
        ok = ok && p.faulted && stale == 0;
    }
    return ok;
}
}  // namespace

bool test_smp()
{
    int n = cpu_count();
    bool ok = n == smp_madt_cpus();
    kprintf("cpus: %d online, MADT lists %d: %s\n", n, smp_madt_cpus(), ok ? "match" : "MISMATCH");
    if (boot_arg("countonly") != nullptr) {
        return ok;
    }
    thread_sleep_ticks(10);
    for (int i = 0; i < n; ++i) {
        Cpu& c = cpu_by_index(i);
        kprintf("cpu%d: APIC id %u, timer interrupts so far %lu, idle thread '%s'\n", c.id,
                c.apic_id, c.ticks, c.idle->name);
        ok = ok && c.ticks > 0;
    }
    done = 0;
    for (int i = 0; i < 4 * n; ++i) {
        thread_create("where", where_am_i, nullptr);
    }
    while (done.load() < 4 * n) {
        thread_sleep_ticks(1);
    }
    kprintf("scheduler: %d unpinned threads ran on CPU mask %x (bit i = cpu i)\n", 4 * n,
            cpus_used.load());

    // B10 again on many CPUs: one large run, then many short runs in a row. The sizes
    // come from the command line (items=, runs=, short=) so that the full curriculum
    // sizes can be run where time allows; the defaults fit the emulator's time limit.
    int half = n / 2 > 0 ? n / 2 : 1;
    ok = producer_consumer(arg_number("items", 200'000), half, half) && ok;
    int runs = int(arg_number("runs", 20)), good = 0;
    uint64_t short_items = arg_number("short", 10'000);
    for (int r = 0; r < runs; ++r) {
        good += producer_consumer(short_items, half, half);
    }
    kprintf("producer/consumer: %d of %d short runs passed\n", good, runs);
    ok = ok && good == runs;

    if (boot_arg("noshootdown") != nullptr) {
        tlb_shootdown_enabled = false;
        kprintf("shootdown: DISABLED for this run (only the unmapping CPU flushes)\n");
    }
    ok = shootdown_test() && ok;
    kprintf("shootdowns performed so far: %lu\n", tlb_shootdown_count());
    return ok;
}

// ------------------------------------------------------------ forensic case (F3-28)
// "Works on 1 CPU, hangs on 8": a bank-transfer routine that locks the source account
// first, then the destination.
namespace {
Spinlock acct_alice{"account:alice"};
Spinlock acct_bob{"account:bob"};
int64_t balance[2] = {1'000'000, 1'000'000};

void transfer(Spinlock& from_lock, int from, Spinlock& to_lock, int to)
{
    uint64_t f = from_lock.lock_irqsave();
    to_lock.lock();
    balance[from] -= 1;
    balance[to] += 1;
    to_lock.unlock();
    from_lock.unlock_irqrestore(f);
}

void teller(void* arg)
{
    auto i = reinterpret_cast<uintptr_t>(arg);
    for (int n = 1; n <= 200'000; ++n) {
        if (i % 2 == 0) {
            transfer(acct_alice, 0, acct_bob, 1);
        } else {
            transfer(acct_bob, 1, acct_alice, 0);
        }
        if (n % 50'000 == 0) {
            kprintf("cpu%d: teller %lu: %d transfers\n", this_cpu().id, i, n);
        }
    }
    done.fetch_add(1);
}
}  // namespace

bool test_abba()
{
    int tellers = 8;
    done = 0;
    for (int i = 0; i < tellers; ++i) {
        thread_create("teller", teller, reinterpret_cast<void*>(uintptr_t(i)));
    }
    while (done.load() < tellers) {
        thread_sleep_ticks(1);
    }
    kprintf("bank: alice %ld, bob %ld, total %ld\n", balance[0], balance[1], balance[0] + balance[1]);
    return balance[0] + balance[1] == 2'000'000;
}

}  // namespace k
