// locks.cc - two harts (CPUs) of uni-rv run the kernel code of two processes that
// both need the same two locks. Build with -DSAME_ORDER for the fixed version.
#include <cstdint>

namespace {
volatile std::uint8_t* const uart = reinterpret_cast<volatile std::uint8_t*>(0x10000000);
volatile std::uint32_t* const finisher = reinterpret_cast<volatile std::uint32_t*>(0x100000);

struct Spinlock {
    const char* name;
    int locked = 0;   // 0 = free, 1 = taken (changed only with atomic instructions)
    int holder = -1;  // which hart holds it, for debugging
};

Spinlock console{"console"};
Spinlock dir_lock{"dir /home"};
Spinlock file_lock{"inode notes.txt"};
int step = 0;  // forces the same interleaving on every run (atomic loads and stores only)
int done = 0;

void raw_acquire(Spinlock& lk, long hart)
{
    while (__atomic_exchange_n(&lk.locked, 1, __ATOMIC_ACQUIRE) != 0) {}
    __atomic_store_n(&lk.holder, static_cast<int>(hart), __ATOMIC_RELAXED);
}
void raw_release(Spinlock& lk)
{
    __atomic_store_n(&lk.holder, -1, __ATOMIC_RELAXED);
    __atomic_store_n(&lk.locked, 0, __ATOMIC_RELEASE);
}

void puts(const char* s)
{
    while (*s != '\0') { *uart = static_cast<std::uint8_t>(*s++); }
}
void putdec(long v)
{
    char buf[24];
    int n = 0;
    do {
        buf[n++] = static_cast<char>('0' + v % 10);
        v /= 10;
    } while (v != 0);
    while (n > 0) { *uart = static_cast<std::uint8_t>(buf[--n]); }
}
// One whole line per call, so the two harts never mix their characters.
void log(long hart, long pid, const char* what, const char* lock_name)
{
    raw_acquire(console, hart);
    puts("cpu");
    putdec(hart);
    puts(" pid ");
    putdec(pid);
    puts(": ");
    puts(what);
    if (lock_name != nullptr) {
        puts(" '");
        puts(lock_name);
        puts("'");
    }
    puts("\n");
    raw_release(console);
}

void acquire(Spinlock& lk, long hart, long pid)
{
    log(hart, pid, "acquire", lk.name);
    long spins = 0;
    while (__atomic_exchange_n(&lk.locked, 1, __ATOMIC_ACQUIRE) != 0) {
        if (++spins == 2000000) {  // a lock-debugging check, like a watchdog
            raw_acquire(console, hart);
            puts("cpu");
            putdec(hart);
            puts(" pid ");
            putdec(pid);
            puts(": still spinning on '");
            puts(lk.name);
            puts("' held by cpu");
            putdec(__atomic_load_n(&lk.holder, __ATOMIC_RELAXED));
            puts("\n");
            raw_release(console);
        }
    }
    __atomic_store_n(&lk.holder, static_cast<int>(hart), __ATOMIC_RELAXED);
    log(hart, pid, "holds", lk.name);
}
void release(Spinlock& lk, long hart, long pid)
{
    log(hart, pid, "release", lk.name);
    raw_release(lk);
}
void set(int& flag, int v) { __atomic_store_n(&flag, v, __ATOMIC_RELEASE); }
void wait_until(const int& flag, int v)
{
    while (__atomic_load_n(&flag, __ATOMIC_ACQUIRE) < v) {}
}
}  // namespace

extern "C" void kmain(long hart)
{
    if (hart == 0) {  // pid 3: rename("/home/notes.txt", ...)
        log(0, 3, "rename: start", nullptr);
        acquire(dir_lock, 0, 3);
        set(step, 1);
        wait_until(step, 2);
        acquire(file_lock, 0, 3);
        log(0, 3, "rename: updating directory entry and inode", nullptr);
        release(file_lock, 0, 3);
        release(dir_lock, 0, 3);
        wait_until(done, 1);  // wait for the other CPU to finish
        log(0, 0, "both system calls finished; powering off", nullptr);
        *finisher = 0x5555;
    } else if (hart == 1) {  // pid 4: unlink("/home/notes.txt")
        wait_until(step, 1);
        log(1, 4, "unlink: start", nullptr);
#ifdef SAME_ORDER
        set(step, 2);
        acquire(dir_lock, 1, 4);
        acquire(file_lock, 1, 4);
        log(1, 4, "unlink: removing directory entry, freeing inode", nullptr);
        release(file_lock, 1, 4);
        release(dir_lock, 1, 4);
#else
        acquire(file_lock, 1, 4);
        set(step, 2);
        acquire(dir_lock, 1, 4);
        log(1, 4, "unlink: removing directory entry, freeing inode", nullptr);
        release(dir_lock, 1, 4);
        release(file_lock, 1, 4);
#endif
        set(done, 1);
    }
    for (;;) { asm volatile("wfi"); }
}
