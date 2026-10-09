// forensic_workers.cpp - F3-51 forensic evidence generator: a small service starts two worker
// processes and logs through stdio. run_lab.sh runs it with standard output going to a file
// (as a service's log would); run.sh runs the same program on a pseudo-terminal for comparison.
#include <cstdio>
#include <cstdlib>
#include <sys/wait.h>
#include <unistd.h>

int main()
{
    std::printf("service: starting worker pool\n");
    for (int w = 1; w <= 2; ++w) {
        pid_t pid = fork();
        if (pid == 0) {
            std::printf("worker %d: ready\n", w);
            std::exit(0);
        }
        int st = 0;
        waitpid(pid, &st, 0);
    }
    std::printf("service: all workers finished\n");
    return 0;
}
