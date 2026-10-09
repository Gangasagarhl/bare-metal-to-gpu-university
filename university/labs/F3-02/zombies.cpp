// zombies.cpp - evidence for the forensic lab: children that exit but are never waited for.
#include <chrono>
#include <cstdio>
#include <fstream>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>

// The third field of /proc/<pid>/stat is the process state letter (see proc(5)).
static std::string state_of(pid_t pid)
{
    std::ifstream f("/proc/" + std::to_string(pid) + "/stat");
    if (!f) { return "no entry (gone)"; }
    std::string pidField, comm, state;
    f >> pidField >> comm >> state;
    return state + " " + comm;
}

int main()
{
    std::vector<pid_t> kids;
    for (int i = 0; i < 3; ++i) {
        std::fflush(stdout);
        const pid_t pid = fork();
        if (pid == 0) {
            _exit(0);  // the print job finishes at once
        }
        kids.push_back(pid);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    std::printf("before waiting (children exited 300 ms ago):\n");
    for (std::size_t i = 0; i < kids.size(); ++i) {
        std::printf("  child %zu: state %s\n", i, state_of(kids[i]).c_str());
    }
    for (pid_t pid : kids) {
        int status = 0;
        waitpid(pid, &status, 0);  // collect the exit status: the entry can go
    }
    std::printf("after waitpid for each child:\n");
    for (std::size_t i = 0; i < kids.size(); ++i) {
        std::printf("  child %zu: state %s\n", i, state_of(kids[i]).c_str());
    }
    return 0;
}
