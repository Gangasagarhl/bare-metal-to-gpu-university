// linux_inversion.cc - BR-05 Listing 11: trap 2, priority inversion with POSIX threads on
// Linux. All threads are pinned to CPU 0 and use SCHED_FIFO, so only one runs at a time and
// the most urgent ready thread always runs (the RTOS rule of F3-39, applied by Linux):
//   L (priority 10) locks the mutex and works 20 ms of CPU time inside it;
//   H (priority 30) arrives 5 ms later and wants the same mutex;
//   M (priority 20) arrives at the same moment, needs no mutex, and works 100 ms.
// --protocol none:    the mutex has no priority protocol (the default);
// --protocol inherit: the mutex uses priority inheritance (PTHREAD_PRIO_INHERIT).
#include <pthread.h>
#include <sched.h>
#include <time.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace {

pthread_mutex_t mutex;
long long hWaitNs = 0;

long long clockNs(clockid_t id)
{
    timespec t{};
    clock_gettime(id, &t);
    return t.tv_sec * 1'000'000'000LL + t.tv_nsec;
}

void burnCpuMs(long long ms)                  // ms of THIS thread's CPU time (not wall time)
{
    const long long end = clockNs(CLOCK_THREAD_CPUTIME_ID) + ms * 1'000'000LL;
    while (clockNs(CLOCK_THREAD_CPUTIME_ID) < end) {
    }
}

void check(int rc, const char* what)
{
    if (rc != 0) {
        std::fprintf(stderr, "%s: %s\n", what, std::strerror(rc));
        std::exit(1);
    }
}

void* low(void*)
{
    check(pthread_mutex_lock(&mutex), "L lock");
    burnCpuMs(20);                            // the critical section
    check(pthread_mutex_unlock(&mutex), "L unlock");
    return nullptr;
}

void* medium(void*)
{
    burnCpuMs(100);                           // unrelated work, no mutex
    return nullptr;
}

void* high(void*)
{
    const long long t0 = clockNs(CLOCK_MONOTONIC);
    check(pthread_mutex_lock(&mutex), "H lock");
    hWaitNs = clockNs(CLOCK_MONOTONIC) - t0;
    check(pthread_mutex_unlock(&mutex), "H unlock");
    return nullptr;
}

pthread_t startThread(void* (*fn)(void*), int priority)
{
    pthread_attr_t a;
    check(pthread_attr_init(&a), "attr_init");
    check(pthread_attr_setinheritsched(&a, PTHREAD_EXPLICIT_SCHED), "setinheritsched");
    check(pthread_attr_setschedpolicy(&a, SCHED_FIFO), "setschedpolicy");
    sched_param p{};
    p.sched_priority = priority;
    check(pthread_attr_setschedparam(&a, &p), "setschedparam");
    pthread_t t{};
    check(pthread_create(&t, &a, fn, nullptr), "pthread_create");
    check(pthread_attr_destroy(&a), "attr_destroy");
    return t;
}

}  // namespace

int main(int argc, char** argv)
{
    const bool inherit = argc == 3 && std::string(argv[1]) == "--protocol" &&
                         std::string(argv[2]) == "inherit";
    cpu_set_t one;
    CPU_ZERO(&one);
    CPU_SET(0, &one);
    check(sched_setaffinity(0, sizeof one, &one) == 0 ? 0 : errno, "sched_setaffinity");
    sched_param p{};
    p.sched_priority = 90;                    // main() must outrank the three threads
    check(pthread_setschedparam(pthread_self(), SCHED_FIFO, &p), "main SCHED_FIFO");

    pthread_mutexattr_t ma;
    check(pthread_mutexattr_init(&ma), "mutexattr_init");
    check(pthread_mutexattr_setprotocol(&ma, inherit ? PTHREAD_PRIO_INHERIT : PTHREAD_PRIO_NONE),
          "mutexattr_setprotocol");
    check(pthread_mutex_init(&mutex, &ma), "mutex_init");

    std::printf("BR-05 trap 2: priority inversion on Linux, one CPU, SCHED_FIFO, protocol %s\n",
                inherit ? "PTHREAD_PRIO_INHERIT" : "PTHREAD_PRIO_NONE (default)");
    const pthread_t l = startThread(low, 10);
    const timespec fiveMs{0, 5'000'000};
    nanosleep(&fiveMs, nullptr);              // L runs and takes the mutex
    const pthread_t h = startThread(high, 30);
    const pthread_t m = startThread(medium, 20);
    check(pthread_join(h, nullptr), "join H");
    check(pthread_join(m, nullptr), "join M");
    check(pthread_join(l, nullptr), "join L");
    check(pthread_mutex_destroy(&mutex), "mutex_destroy");
    check(pthread_mutexattr_destroy(&ma), "mutexattr_destroy");

    const long long waitMs = (hWaitNs + 500'000) / 1'000'000;
    std::printf("L's critical section: 20 ms of CPU, of which about 15 ms remain when H arrives\n");
    std::printf("M's unrelated work:   100 ms of CPU\n");
    std::printf("H waited for the mutex: %lld ms (rounded)\n", waitMs);
    std::printf("verdict: %s\n", waitMs > 50 ? "H also waited for M (priority inversion)"
                                             : "H waited only for L's critical section");
    return 0;
}
