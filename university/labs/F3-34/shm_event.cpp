// shm_event.cpp - F3-34: two processes hand 1,000,000 numbers to each other through one shared
// memory slot, coordinated by two events (curriculum B17, second acceptance test, run on the
// host's kernel). The events are POSIX semaphores placed inside the shared memory itself.
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

struct Shared
{
    sem_t full;                  // event "a number is in the slot"
    sem_t empty;                 // event "the slot may be overwritten"
    std::uint64_t slot;          // the shared data: one number at a time
    std::uint64_t lost, received;
};

int main()
{
    const std::uint64_t handoffs = 1000000;
    void* mem = mmap(nullptr, sizeof(Shared), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) { std::perror("mmap"); return 1; }
    auto* s = static_cast<Shared*>(mem);
    s->slot = s->lost = s->received = 0;
    // pshared = 1: the semaphore may be used by any process that maps this memory.
    if (sem_init(&s->full, 1, 0) != 0 || sem_init(&s->empty, 1, 1) != 0) { std::perror("sem_init"); return 1; }
    auto t0 = std::chrono::steady_clock::now();
    pid_t child = fork();
    if (child == 0) {                                     // consumer
        for (std::uint64_t i = 1; i <= handoffs; ++i) {
            while (sem_wait(&s->full) != 0) { }           // retry if interrupted by a signal
            if (s->slot != i) ++s->lost;                  // every number must arrive, in order
            ++s->received;
            sem_post(&s->empty);
        }
        _exit(0);
    }
    for (std::uint64_t i = 1; i <= handoffs; ++i) {       // producer
        while (sem_wait(&s->empty) != 0) { }
        s->slot = i;
        sem_post(&s->full);
    }
    int st = 0;
    waitpid(child, &st, 0);
    double secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    std::printf("handoffs %llu, received %llu, lost or out of order %llu\n",
                static_cast<unsigned long long>(handoffs), static_cast<unsigned long long>(s->received),
                static_cast<unsigned long long>(s->lost));
    std::printf("measured in this build container (varies run to run): %.2f s, %.2f us per handoff\n",
                secs, secs * 1e6 / static_cast<double>(handoffs));
    bool ok = WIFEXITED(st) && WEXITSTATUS(st) == 0 && s->received == handoffs && s->lost == 0;
    std::printf("B17 shared-memory test: %s\n", ok ? "PASS (1 million handoffs without loss)" : "FAIL");
    sem_destroy(&s->full);
    sem_destroy(&s->empty);
    munmap(mem, sizeof(Shared));
    return ok ? 0 : 1;
}
