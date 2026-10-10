// measure.cpp - F3-55: how much a compiler asks of an operating system, measured. Runs the
// host's g++ on the U3 test program (F3-53) as a child process and reports the child's exit
// status, wall-clock time, CPU time and the peak memory of the largest process in its tree.
// The numbers are measurements of this run on this machine, not properties of g++.
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string_view>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

int main()
{
    // Calibration: a child that touches 64 MiB, to check the unit of ru_maxrss on this system.
    pid_t cal = fork();
    if (cal == 0) {
        std::vector<char> block(64u << 20);
        std::memset(block.data(), 1, block.size());
        _exit(block[12345] == 1 ? 0 : 1);
    }
    waitpid(cal, nullptr, 0);
    rusage cal_ru {};
    getrusage(RUSAGE_CHILDREN, &cal_ru);
    std::printf("calibration: a child that touched 64 MiB has ru_maxrss = %ld, so the unit is KiB\n",
                cal_ru.ru_maxrss);

    char dir[] = "/tmp/os402-measure-XXXXXX";
    if (mkdtemp(dir) == nullptr) return 2;
    std::string_view src = "../F3-53/u3_test.cc";
    char out[64];
    std::snprintf(out, sizeof out, "%s/u3_test.o", dir);
    auto t0 = std::chrono::steady_clock::now();
    pid_t pid = fork();
    if (pid == 0) {
        execlp("g++", "g++", "-std=c++20", "-O2", "-c", src.data(), "-o", out, static_cast<char*>(nullptr));
        _exit(127);                                   // only reached if exec failed
    }
    int st = 0;
    waitpid(pid, &st, 0);
    auto t1 = std::chrono::steady_clock::now();
    rusage ru {};
    getrusage(RUSAGE_CHILDREN, &ru);                  // all waited-for descendants, including cc1plus
                                                      // (and the calibration child: see the check)
    unlink(out);
    rmdir(dir);
    double wall = std::chrono::duration<double>(t1 - t0).count();
    double user = static_cast<double>(ru.ru_utime.tv_sec) + static_cast<double>(ru.ru_utime.tv_usec) / 1e6;
    double sys = static_cast<double>(ru.ru_stime.tv_sec) + static_cast<double>(ru.ru_stime.tv_usec) / 1e6;
    std::printf("command: g++ -std=c++20 -O2 -c %s\n", src.data());
    std::printf("exit status: %d\n", WIFEXITED(st) ? WEXITSTATUS(st) : -1);
    std::printf("wall-clock time: %.2f s; CPU time: user %.2f s, system %.2f s\n", wall, user, sys);
    std::printf("peak resident memory of the largest process: %ld MiB%s\n", ru.ru_maxrss / 1024,
                ru.ru_maxrss > cal_ru.ru_maxrss ? "" : " (not above the calibration child: inconclusive)");
    std::printf("(measured in this run, on this build container; your machine will differ)\n");
    return WIFEXITED(st) && WEXITSTATUS(st) == 0 ? 0 : 1;
}
