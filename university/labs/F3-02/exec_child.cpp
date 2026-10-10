// exec_child.cpp - the shell's pattern: fork a child, the child execs a new program,
// and the parent waits.
#include <cstdio>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main()
{
    std::printf("parent: about to run: echo timetable printed\n");
    std::fflush(stdout);
    const pid_t child = fork();
    if (child < 0) {
        std::perror("fork");
        return 1;
    }
    if (child == 0) {
        // Replace this process's program with /bin/echo. On success execv never returns.
        char prog[] = "/bin/echo";
        char arg1[] = "timetable";
        char arg2[] = "printed";
        char* argv[] = {prog, arg1, arg2, nullptr};
        execv(prog, argv);
        std::perror("execv");  // only reached if exec failed
        _exit(127);
    }
    int status = 0;
    waitpid(child, &status, 0);
    std::printf("parent: child finished, exit status %d\n",
                WIFEXITED(status) ? WEXITSTATUS(status) : -1);
    return 0;
}
