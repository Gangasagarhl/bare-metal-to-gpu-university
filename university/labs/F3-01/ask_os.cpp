// ask_os.cpp - a program cannot see the machine directly; it asks the operating system.
#include <cstdio>
#include <sys/utsname.h>
#include <unistd.h>

int main()
{
    std::printf("my process id (pid):        %d\n", static_cast<int>(getpid()));
    std::printf("my parent's process id:     %d\n", static_cast<int>(getppid()));
    std::printf("CPUs the OS says are online: %ld\n", sysconf(_SC_NPROCESSORS_ONLN));
    std::printf("page size the OS uses:      %ld bytes\n", sysconf(_SC_PAGESIZE));

    utsname info{};
    if (uname(&info) == 0) {
        std::printf("operating system kernel:    %s %s (%s)\n", info.sysname, info.release,
                    info.machine);
    }
    return 0;
}
