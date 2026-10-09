// fork_count.cpp - how many processes do three fork() calls in a row create?
// Every process writes one byte into a shared pipe; the original counts the bytes.
#include <cstdio>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main()
{
    int fds[2];
    if (pipe(fds) != 0) {
        std::perror("pipe");
        return 1;
    }
    const pid_t original = getpid();
    for (int i = 0; i < 3; ++i) {
        std::fflush(stdout);
        if (fork() < 0) {
            std::perror("fork");
            return 1;
        }
    }
    const char one = '1';
    if (write(fds[1], &one, 1) != 1) {  // every process: "I exist"
        return 1;
    }
    close(fds[1]);
    if (getpid() != original) {
        _exit(0);  // all copies except the original stop here
    }
    int count = 0;
    char c = 0;
    while (read(fds[0], &c, 1) == 1) {  // ends when no process holds the write end any more
        ++count;
    }
    while (wait(nullptr) > 0) {}  // collect the original's own children
    std::printf("three fork() calls in a row -> %d processes in total\n", count);
    return 0;
}
