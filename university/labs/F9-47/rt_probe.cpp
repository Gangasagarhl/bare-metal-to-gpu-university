// rt_probe.cpp - F9-47 Listing 1: what does this Linux kernel offer a real-time program?
// Reads only standard interfaces: uname(2), /sys and /proc files, sched_* and getrlimit(2).
#include <dirent.h>
#include <sched.h>
#include <sys/resource.h>
#include <sys/utsname.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>

namespace {

const char* policyName(int p)
{
    switch (p) {
    case SCHED_OTHER: return "SCHED_OTHER";
    case SCHED_FIFO: return "SCHED_FIFO";
    case SCHED_RR: return "SCHED_RR";
    default: return "other";
    }
}

std::string limitText(rlim_t v)
{
    return v == RLIM_INFINITY ? std::string("unlimited") : std::to_string(v);
}

}  // namespace

int main()
{
    utsname u{};
    uname(&u);
    std::printf("kernel release: %s\n", u.release);
    std::printf("kernel version string: %s\n", u.version);

    std::ifstream rt("/sys/kernel/realtime");
    std::printf("/sys/kernel/realtime: %s\n", rt ? "present" : "absent");

    std::printf("SCHED_FIFO priorities: %d..%d\n", sched_get_priority_min(SCHED_FIFO),
                sched_get_priority_max(SCHED_FIFO));
    rlimit lim{};
    getrlimit(RLIMIT_RTPRIO, &lim);
    std::printf("RLIMIT_RTPRIO (soft/hard): %s/%s\n", limitText(lim.rlim_cur).c_str(),
                limitText(lim.rlim_max).c_str());
    getrlimit(RLIMIT_MEMLOCK, &lim);
    std::printf("RLIMIT_MEMLOCK bytes (soft/hard): %s/%s\n", limitText(lim.rlim_cur).c_str(),
                limitText(lim.rlim_max).c_str());

    // Can this process become a real-time task? Try, report, and go back.
    sched_param sp{};
    sp.sched_priority = 80;
    if (sched_setscheduler(0, SCHED_FIFO, &sp) == 0) {
        std::printf("sched_setscheduler(SCHED_FIFO, 80): ok, policy now %s\n",
                    policyName(sched_getscheduler(0)));
        sp.sched_priority = 0;
        sched_setscheduler(0, SCHED_OTHER, &sp);
    } else {
        std::printf("sched_setscheduler(SCHED_FIFO, 80): failed: %s\n", std::strerror(errno));
    }

    // Interrupt handlers that run as kernel threads are named "irq/<number>-<name>".
    int irqThreads = 0;
    DIR* proc = opendir("/proc");
    for (dirent* e = proc ? readdir(proc) : nullptr; e != nullptr; e = readdir(proc)) {
        if (e->d_name[0] < '0' || e->d_name[0] > '9') { continue; }
        std::ifstream comm(std::string("/proc/") + e->d_name + "/comm");
        std::string name;
        std::getline(comm, name);
        if (name.rfind("irq/", 0) != 0) { continue; }
        const int pid = std::stoi(e->d_name);
        sched_param p{};
        sched_getparam(pid, &p);
        std::printf("  interrupt thread %-22s %s priority %d\n", name.c_str(),
                    policyName(sched_getscheduler(pid)), p.sched_priority);
        ++irqThreads;
    }
    if (proc != nullptr) { closedir(proc); }

    int irqLines = 0;
    std::ifstream interrupts("/proc/interrupts");
    for (std::string line; std::getline(interrupts, line);) {
        const auto colon = line.find(':');
        if (colon == std::string::npos) { continue; }
        const std::string head = line.substr(0, colon);
        if (head.find_first_not_of(' ') != std::string::npos &&
            head.find_first_not_of(" 0123456789") == std::string::npos) {
            ++irqLines;
        }
    }
    std::printf("numbered interrupt lines in /proc/interrupts: %d; handled by threads: %d\n",
                irqLines, irqThreads);
    return 0;
}
