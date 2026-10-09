// ns_demo.cpp - the two kernel features under every Linux container, seen from C++:
// a new UTS namespace (own host name) and a new PID namespace (own process numbers).
// Needs root (or CAP_SYS_ADMIN); prints the kernel's error message otherwise (F5-24, DS303).
#include <sched.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <string>

std::string nsId(const char* kind)
{
    const std::string path = std::string("/proc/self/ns/") + kind;
    char buf[64] = {};
    const ssize_t n = readlink(path.c_str(), buf, sizeof(buf) - 1);
    return n > 0 ? std::string(buf, static_cast<std::size_t>(n)) : std::string("?");
}

std::string hostName()
{
    char buf[65] = {};
    gethostname(buf, sizeof(buf) - 1);
    return buf;
}

// Runs in a helper process, so the parent keeps its own namespaces.
int helper()
{
    if (unshare(CLONE_NEWUTS | CLONE_NEWPID) != 0) {
        std::printf("unshare failed: %s\n", std::strerror(errno));
        return 1;
    }
    const pid_t child = fork();             // the first child is pid 1 in the new namespace
    if (child < 0) {
        std::printf("fork failed: %s\n", std::strerror(errno));
        return 1;
    }
    if (child == 0) {
        const char name[] = "box1";
        sethostname(name, sizeof(name) - 1);  // changes the name in the new UTS namespace only
        std::printf("child inside:  pid %d, parent pid %d, host %s, %s, %s\n",
                    static_cast<int>(getpid()), static_cast<int>(getppid()), hostName().c_str(),
                    nsId("uts").c_str(), nsId("pid").c_str());
        std::fflush(stdout);
        _exit(0);
    }
    int status = 0;
    waitpid(child, &status, 0);
    std::printf("helper:        sees that child as pid %d; child exit status %d\n",
                static_cast<int>(child), WIFEXITED(status) ? WEXITSTATUS(status) : -1);
    return 0;
}

int main()
{
    std::printf("parent before: pid %d, host %s, %s, %s\n", static_cast<int>(getpid()),
                hostName().c_str(), nsId("uts").c_str(), nsId("pid").c_str());
    std::fflush(stdout);
    const pid_t h = fork();
    if (h == 0) {
        const int rc = helper();
        std::fflush(stdout);
        _exit(rc);                          // skip the sanitizer's exit checks in the helper
    }
    int status = 0;
    waitpid(h, &status, 0);
    std::printf("parent after:  pid %d, host still %s, %s\n", static_cast<int>(getpid()),
                hostName().c_str(), nsId("uts").c_str());
    return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
}
