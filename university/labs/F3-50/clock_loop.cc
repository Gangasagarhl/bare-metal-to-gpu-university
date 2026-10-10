// clock_loop.cc - F3-50: asks for the time 1,000 times. run.sh counts, with strace, how many
// of those requests actually entered the kernel as clock_gettime system calls.
#include <cstdio>
#include <ctime>

int main()
{
    timespec t {};
    long ok = 0;
    for (int i = 0; i < 1000; ++i) {
        ok += clock_gettime(CLOCK_MONOTONIC, &t) == 0 ? 1 : 0;
    }
    std::printf("clock_gettime succeeded %ld times\n", ok);
    return 0;
}
