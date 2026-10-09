// test_proc.cc - the B13 acceptance tests: init and ten children, 10,000 exec/exit
// cycles, and a fuzzing run of the exec path.
#include "hooks.h"
#include "process.h"

namespace k {

namespace {
struct Stats {
    uint64_t frames, objects;
    int threads, procs;
};
Stats snapshot()
{
    thread_reap_zombies();
    return Stats{frames_free(), heap_live_objects(), threads_live(), processes_live()};
}
void print(const char* label, const Stats& s)
{
    kprintf("%s: free frames %lu, heap objects %lu, threads %d, processes %d\n", label, s.frames,
            s.objects, s.threads, s.procs);
}
bool same(const Stats& a, const Stats& b)
{
    return a.frames == b.frames && a.objects == b.objects && a.threads == b.threads &&
           a.procs == b.procs;
}
int spawn_and_wait(const char* module, const char* const* argv, int argc)
{
    int64_t pid = process_spawn_module(module, argv, argc, nullptr);
    if (pid < 0) {
        return int(pid);
    }
    int status = -1;
    process_wait(nullptr, &status);
    return status;
}
}  // namespace

bool test_proc()
{
    Stats base = snapshot();
    print("before", base);
    const char* args[] = {"u_args.elf", "hello", "world"};
    int st = spawn_and_wait("u_args.elf", args, 3);
    kprintf("kernel: u_args exited with status %d\n", st);
    bool ok = st == 0;

    const char* init_argv[] = {"u_init.elf"};
    st = spawn_and_wait("u_init.elf", init_argv, 1);
    kprintf("kernel: init exited with status %d\n", st);
    ok = ok && st == 0;

    const char* spin_argv[] = {"u_spin.elf"};
    int64_t pid = process_spawn_module("u_spin.elf", spin_argv, 1, nullptr);
    thread_sleep_ticks(20);
    process_kill(pid);
    int status = 0;
    process_wait(nullptr, &status);
    kprintf("kernel: u_spin (pid %ld) killed after 20 ticks, status 0x%x\n", pid, status);
    ok = ok && status == kStatusKilledByKernel;

    Stats after = snapshot();
    print("after", after);
    return ok && same(base, after);
}

bool test_execloop()
{
    const char* argv[] = {"u_nop.elf"};
    spawn_and_wait("u_nop.elf", argv, 1);   // warm-up
    Stats base = snapshot();
    print("baseline", base);
    bool ok = true;
    for (int i = 1; i <= 10'000; ++i) {
        ok = spawn_and_wait("u_nop.elf", argv, 1) == 0 && ok;
        if (i % 1000 == 0) {
            char label[32] = "after       exec/exit";
            for (int v = i, p = 10; v > 0; v /= 10, --p) {
                label[p] = char('0' + v % 10);
            }
            print(label, snapshot());
        }
    }
    Stats after = snapshot();
    ok = ok && same(base, after);
    kprintf("exec/exit: %s\n", same(base, after) ? "memory back at the baseline" : "LEAK");
    return ok;
}

// ---------------------------------------------------------------- fuzzing the exec path
namespace {
uint8_t fuzz_buf[64 * 1024];
uint64_t rng = 0x0123456789ABCDEFull;   // fixed seed: the run is repeatable
uint64_t next_random()                  // xorshift64
{
    rng ^= rng << 13;
    rng ^= rng >> 7;
    rng ^= rng << 17;
    return rng;
}
}  // namespace

bool test_fuzz()
{
    const BootModule* seed = boot_module_find("u_child.elf");
    if (seed == nullptr || seed->size > sizeof(fuzz_buf)) {
        return false;
    }
    const char* argv[] = {"fuzzed", "7"};
    spawn_and_wait("u_child.elf", argv, 2);   // warm-up
    Stats base = snapshot();
    print("baseline", base);
    const int kRuns = 3000;
    int rejected[32] = {};
    int exited = 0, faulted = 0, timed_out = 0;
    console_set_quiet(true);   // fuzzed programs may print garbage
    for (int run = 0; run < kRuns; ++run) {
        memcpy(fuzz_buf, seed->data, seed->size);
        int flips = 1 + int(next_random() % 4);
        for (int i = 0; i < flips; ++i) {
            // Mostly the headers (first 256 bytes), sometimes anywhere in the file.
            uint64_t span = (next_random() % 4 != 0) ? 256 : seed->size;
            fuzz_buf[next_random() % span] = uint8_t(next_random());
        }
        ElfImage img;
        ElfError e = elf_check(fuzz_buf, seed->size, kUserBase, kUserStackTop - kUserStackSize, img);
        if (e != ElfError::None) {
            rejected[int(e)]++;
            continue;
        }
        int64_t pid = process_spawn(fuzz_buf, seed->size, "fuzzed", argv, 2, nullptr);
        if (pid < 0) {
            rejected[0]++;
            continue;
        }
        int status = -1;
        for (int t = 0; t < 30 && !process_try_wait(nullptr, &status); ++t) {
            thread_sleep_ticks(1);
        }
        if (status == -1) {
            process_kill(int(pid));
            process_wait(nullptr, &status);
        }
        if (status == kStatusKilledByKernel) {
            ++timed_out;
        } else if (status & kStatusKilled) {
            ++faulted;
        } else {
            ++exited;
        }
    }
    console_set_quiet(false);
    int nrej = 0;
    kprintf("fuzz: %d mutated executables through the exec path\n", kRuns);
    for (int i = 1; i < 32 && i <= int(ElfError::EntryNotExecutable); ++i) {
        if (rejected[i]) {
            kprintf("fuzz:   rejected (%s): %d\n", elf_error_name(ElfError(i)), rejected[i]);
            nrej += rejected[i];
        }
    }
    kprintf("fuzz:   loaded and exited normally: %d\n", exited);
    kprintf("fuzz:   loaded, killed by a CPU exception in ring 3: %d\n", faulted);
    kprintf("fuzz:   loaded, killed after the 30-tick time limit: %d\n", timed_out);
    kprintf("fuzz: rejected %d, ran %d, kernel faults 0 (we are still here)\n", nrej,
            exited + faulted + timed_out);
    Stats after = snapshot();
    print("after", after);
    return same(base, after) && nrej + exited + faulted + timed_out == kRuns;
}

}  // namespace k
