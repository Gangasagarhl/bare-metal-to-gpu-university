// noisy.cpp - BR-10 Listing 5: the trap "benchmarking on a noisy shared node".
// Part A asks how much CPU this machine really gives: k copies of the same work at once.
// Part B puts three busy neighbour processes on the CPU of node n2 and times the job
// with fixed and with dynamic task assignment, quiet and noisy.
#include "bench.hpp"
#include "cluster.hpp"
#include <thread>

namespace
{

pid_t startHog(int cpu)  // a neighbour that only burns CPU time, pinned to one CPU
{
    pid_t const pid = ::fork();
    if (pid == 0) {
        prctl(PR_SET_PDEATHSIG, SIGKILL);
        mini::pinTo(cpu);
        unsigned long long x = 1;
        while (true) {
            x = x * 6364136223846793005ULL + 1;
            bench::keep(x);
        }
    }
    return pid;
}

void stopHogs(std::vector<pid_t> const& hogs)
{
    for (pid_t p : hogs) {
        ::kill(p, SIGKILL);
        ::waitpid(p, nullptr, 0);
    }
}

constexpr std::uint64_t kEnd = 1000000;  // 78498 primes

}  // namespace

int main()
{
    std::printf("A. how many CPUs of work does this machine really run at once?\n");
    std::printf("   %u logical CPUs visible. k processes each time the same job, 7 runs:\n",
                std::thread::hardware_concurrency());
    double single = 0;
    for (int k = 1; k <= 4; ++k) {
        std::vector<int> pipes;
        std::vector<pid_t> kids;
        for (int j = 0; j < k; ++j) {
            int fd[2];
            if (::pipe(fd) != 0) {
                return 1;
            }
            std::fflush(stdout);
            pid_t const pid = ::fork();
            if (pid == 0) {
                ::close(fd[0]);
                auto const s = bench::run([] { bench::keep(job::countPrimes(0, kEnd).sum); }, 2, 7);
                ssize_t const w = ::write(fd[1], &s.median, sizeof s.median);
                _exit(w == sizeof s.median ? 0 : 1);
            }
            ::close(fd[1]);
            pipes.push_back(fd[0]);
            kids.push_back(pid);
        }
        double worst = 0;
        for (int j = 0; j < k; ++j) {
            double m = 0;
            if (::read(pipes[static_cast<std::size_t>(j)], &m, sizeof m) != sizeof m) {
                return 1;
            }
            ::close(pipes[static_cast<std::size_t>(j)]);
            ::waitpid(kids[static_cast<std::size_t>(j)], nullptr, 0);
            worst = std::max(worst, m);
        }
        if (k == 1) {
            single = worst;
        }
        std::printf("   k=%d  slowest median %6.2f ms  -> CPUs' worth of work done at once: %.2f\n",
                    k, worst * 1e3, k * single / worst);
    }

    std::printf("\nB. the job on three nodes pinned to CPUs 0, 1, 2; three neighbours on CPU 1 (n2)\n");
    mini::pinTo(3);  // the controller keeps CPU 3 for itself
    std::printf("   median of 11 runs after 3 warm-ups; per-node lines are from the last run\n");
    for (bool noisy : {false, true}) {
        for (mini::Assign a : {mini::Assign::fixed, mini::Assign::dynamic}) {
            mini::Config cfg;
            cfg.jobEnd = kEnd;
            cfg.log = false;
            cfg.assign = a;
            for (int i = 0; i < 3; ++i) {
                cfg.nodes[static_cast<std::size_t>(i)].cpu = i;
            }
            mini::Cluster c(cfg);
            std::vector<pid_t> hogs;
            if (noisy) {
                hogs = {startHog(1), startHog(1), startHog(1)};
            }
            mini::JobResult last;
            bool ok = true;
            auto const s = bench::run([&] {
                last = c.runJob();
                ok = ok && last.finished && last.total.count == 78498;
            }, 3, 11);
            stopHogs(hogs);
            std::printf("   %s %s: job median %6.2f ms (min %6.2f, max %6.2f)%s\n",
                        noisy ? "noisy" : "quiet", a == mini::Assign::fixed ? "fixed  " : "dynamic",
                        s.median * 1e3, s.minimum * 1e3, s.maximum * 1e3, ok ? "" : "  WRONG");
            for (std::size_t i = 0; i < 3; ++i) {
                std::printf("       n%zu: %d tasks, busy %5.1f ms on its clock, waited for %5.1f ms"
                            " by the controller\n", i + 1, last.perNode[i].tasks,
                            last.perNode[i].busyMs, last.perNode[i].replyMs);
            }
        }
    }
    return 0;
}
