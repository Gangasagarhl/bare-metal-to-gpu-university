// fork_wait.cpp - one process becomes two; each has its own copy of the variables.
#include <cstdio>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main()
{
    int marks = 10;              // one variable, before the split
    const pid_t me = getpid();   // remember who the parent is
    std::fflush(stdout);         // empty the output buffer before copying the process
    const pid_t child = fork();  // from here on, two processes run this code
    if (child < 0) {
        std::perror("fork");
        return 1;
    }
    if (child == 0) {  // fork returned 0: we are the new (child) process
        marks = 99;    // changes the child's copy only
        std::printf("child:  fork returned 0, my marks = %d\n", marks);
        std::printf("child:  my parent is the process that called fork: %s\n",
                    getppid() == me ? "yes" : "no");
        return 7;  // the child's exit status
    }
    int status = 0;  // fork returned the child's pid: we are the parent
    const pid_t done = waitpid(child, &status, 0);  // sleep until that child exits
    std::printf("parent: fork returned the child's pid, waitpid returned the same pid: %s\n",
                done == child ? "yes" : "no");
    if (WIFEXITED(status)) {
        std::printf("parent: child exited with status %d\n", WEXITSTATUS(status));
    }
    std::printf("parent: my marks = %d (the child's change did not reach me)\n", marks);
    return 0;
}
