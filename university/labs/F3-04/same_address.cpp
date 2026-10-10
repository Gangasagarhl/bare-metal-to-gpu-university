// same_address.cpp - after fork, parent and child use the SAME virtual address
// for a variable, yet see DIFFERENT values: each process has its own address space.
#include <cstdio>
#include <sys/wait.h>
#include <unistd.h>

int seat = 1;  // a global variable: one fixed virtual address in the program

int main()
{
    std::fflush(stdout);
    const pid_t child = fork();
    if (child < 0) {
        std::perror("fork");
        return 1;
    }
    if (child == 0) {
        seat = 42;  // only the child's copy changes
        std::printf("child:  &seat = %p, seat = %d\n", static_cast<void*>(&seat), seat);
        return 0;
    }
    waitpid(child, nullptr, 0);  // let the child finish first
    std::printf("parent: &seat = %p, seat = %d\n", static_cast<void*>(&seat), seat);
    return 0;
}
