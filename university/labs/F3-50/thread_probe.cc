// thread_probe.cc - F3-50: starts one thread. run.sh runs it under strace with clone3 made to
// fail with ENOSYS, to show how a C library probes for a newer system call and falls back.
#include <cstdio>
#include <thread>

int main()
{
    int value = 0;
    std::thread t([&value] { value = 7; });
    t.join();
    std::printf("thread ran and set value = %d\n", value);
    return 0;
}
