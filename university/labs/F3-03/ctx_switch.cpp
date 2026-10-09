// ctx_switch.cpp - ask the kernel how often it switched this process off the CPU.
#include <chrono>
#include <cstdio>
#include <sys/resource.h>
#include <thread>

static long voluntary()
{
    rusage u{};
    getrusage(RUSAGE_SELF, &u);
    return u.ru_nvcsw;  // switches because the process waited (blocked)
}

int main()
{
    const long before = voluntary();
    for (int i = 0; i < 50; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));  // block: give the CPU away
    }
    const long slept = voluntary() - before;
    std::printf("voluntary context switches during 50 sleeps: %ld\n", slept);
    std::printf("at least one per sleep:                      %s\n", slept >= 50 ? "yes" : "no");
    return 0;
}
