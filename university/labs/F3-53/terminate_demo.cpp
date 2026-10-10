// terminate_demo.cpp - F3-53: the second U3 acceptance test, on the host as the reference.
// "An uncaught exception terminates the process with the standard termination path; the kernel
// and other processes are unaffected." A child process throws and nobody catches; the parent
// collects the child's error output and status, then carries on.
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <sys/wait.h>
#include <unistd.h>

int main()
{
    int err[2];
    if (pipe(err) != 0) return 2;
    std::fflush(stdout);
    pid_t pid = fork();
    if (pid == 0) {
        dup2(err[1], 2);                           // the child's standard error goes to the pipe
        close(err[0]);
        close(err[1]);
        throw std::runtime_error("nobody catches this");
    }
    close(err[1]);
    char buf[512];
    size_t len = 0;
    ssize_t n;
    while (len < sizeof buf - 1 && (n = read(err[0], buf + len, sizeof buf - 1 - len)) > 0) {
        len += static_cast<size_t>(n);             // read until the child closes its end
    }
    buf[len] = '\0';
    close(err[0]);
    int st = 0;
    waitpid(pid, &st, 0);
    std::printf("child's standard error:\n%s", buf);
    if (WIFSIGNALED(st)) {
        std::printf("child ended by signal %d (%s)\n", WTERMSIG(st), strsignal(WTERMSIG(st)));
    } else {
        std::printf("child exited with status %d\n", WEXITSTATUS(st));
    }
    std::printf("parent still running: 6 * 7 = %d\n", 6 * 7);
    return 0;
}
