// threads_share.cpp - threads of one process share its memory; each has its own stack and id.
#include <atomic>
#include <cstdio>
#include <sys/syscall.h>
#include <thread>
#include <unistd.h>

std::atomic<int> handedIn{0};  // one counter in the process's memory, seen by every thread

struct Seen {
    pid_t pid = 0;  // which process the thread belongs to
    pid_t tid = 0;  // the kernel's id for this thread
    const void* counter = nullptr;
    const void* local = nullptr;
};

static void student(Seen& out)
{
    int myLocal = 0;  // lives on this thread's own stack
    out.pid = getpid();
    out.tid = static_cast<pid_t>(syscall(SYS_gettid));
    out.counter = &handedIn;
    out.local = &myLocal;
    for (int i = 0; i < 1000; ++i) {
        handedIn.fetch_add(1);  // atomic: no lost updates (F1-30)
    }
}

int main()
{
    Seen a, b;
    std::thread t1(student, std::ref(a));
    std::thread t2(student, std::ref(b));
    t1.join();
    t2.join();
    std::printf("same process id for both threads:     %s\n",
                a.pid == b.pid && a.pid == getpid() ? "yes" : "no");
    std::printf("different thread ids:                 %s\n", a.tid != b.tid ? "yes" : "no");
    std::printf("same address for the shared counter:  %s\n",
                a.counter == b.counter ? "yes" : "no");
    std::printf("different addresses for local vars:   %s\n", a.local != b.local ? "yes" : "no");
    std::printf("counter after 2 x 1000 increments:    %d\n", handedIn.load());
    return 0;
}
