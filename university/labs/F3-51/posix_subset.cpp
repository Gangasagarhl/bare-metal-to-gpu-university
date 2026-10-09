// posix_subset.cpp - F3-51: a miniature conformance harness. Each test checks one behaviour
// that POSIX (or the C standard) requires of an interface; the harness prints PASS or FAIL per
// test, the interfaces it covers, and the excluded areas with their reasons. Run here against
// the host's C library (the reference); a port runs the same file against its own library.
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <poll.h>
#include <set>
#include <signal.h>
#include <string>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <vector>

namespace {

std::string dir;                                  // a fresh temporary folder for file tests

std::string path(const char* name) { return dir + "/" + name; }

bool t_snprintf_truncates()
{
    char buf[4];
    volatile int value = 123456;                    // volatile: the compiler cannot pre-compute it
    int n = std::snprintf(buf, sizeof buf, "%d", value);
    return n == 6 && std::strcmp(buf, "123") == 0;   // returns the length it WOULD have written
}

bool t_strtol_range()
{
    errno = 0;
    long v = std::strtol("99999999999999999999999", nullptr, 10);
    return v == LONG_MAX && errno == ERANGE;
}

bool t_open_excl()
{
    int fd = open(path("a").c_str(), O_CREAT | O_EXCL | O_WRONLY, 0644);
    if (fd < 0) return false;
    close(fd);
    int again = open(path("a").c_str(), O_CREAT | O_EXCL | O_WRONLY, 0644);
    return again == -1 && errno == EEXIST;
}

bool t_lowest_descriptor()
{
    int a = open(path("a").c_str(), O_RDONLY);
    int b = open(path("a").c_str(), O_RDONLY);
    close(a);
    int c = open(path("a").c_str(), O_RDONLY);    // must reuse the lowest free number
    bool ok = (c == a);
    close(b);
    close(c);
    return ok;
}

bool t_pipe_eof()
{
    int p[2];
    if (pipe(p) != 0) return false;
    close(p[1]);                                    // no writer left
    char ch;
    bool ok = read(p[0], &ch, 1) == 0;              // end of file, not an error
    close(p[0]);
    return ok;
}

bool t_pipe_epipe()
{
    int p[2];
    if (pipe(p) != 0) return false;
    close(p[0]);                                    // no reader left
    struct sigaction ign {}, old {};
    ign.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &ign, &old);
    errno = 0;
    bool ok = write(p[1], "x", 1) == -1 && errno == EPIPE;
    sigaction(SIGPIPE, &old, nullptr);
    close(p[1]);
    return ok;
}

bool t_dup2_shares_offset()
{
    int fd = open(path("b").c_str(), O_CREAT | O_RDWR | O_TRUNC, 0644);
    int fd2 = dup2(fd, 20);
    bool ok = fd2 == 20 && write(fd, "abc", 3) == 3 && lseek(fd2, 0, SEEK_CUR) == 3;
    close(fd);
    close(fd2);
    return ok;
}

bool t_readdir()
{
    DIR* d = opendir(dir.c_str());
    if (d == nullptr) return false;
    std::set<std::string> names;
    while (dirent* e = readdir(d)) names.insert(e->d_name);
    closedir(d);
    return names == std::set<std::string>{".", "..", "a", "b"};
}

bool t_monotonic()
{
    timespec prev {}, now {};
    clock_gettime(CLOCK_MONOTONIC, &prev);
    for (int i = 0; i < 1000; ++i) {
        clock_gettime(CLOCK_MONOTONIC, &now);
        if (now.tv_sec < prev.tv_sec || (now.tv_sec == prev.tv_sec && now.tv_nsec < prev.tv_nsec)) {
            return false;
        }
        prev = now;
    }
    return true;
}

bool t_poll_timeout()
{
    int p[2];
    if (pipe(p) != 0) return false;
    pollfd f {p[0], POLLIN, 0};
    bool ok = poll(&f, 1, 10) == 0;                 // nothing to read: times out, returns 0
    close(p[0]);
    close(p[1]);
    return ok;
}

bool t_wait_status()
{
    pid_t pid = fork();
    if (pid == 0) _exit(3);
    int st = 0;
    return pid > 0 && waitpid(pid, &st, 0) == pid && WIFEXITED(st) && WEXITSTATUS(st) == 3;
}

bool t_environment()
{
    setenv("OS402_TEST", "one", 1);
    bool a = std::strcmp(std::getenv("OS402_TEST"), "one") == 0;
    setenv("OS402_TEST", "two", 0);                 // overwrite = 0: keep the old value
    bool b = std::strcmp(std::getenv("OS402_TEST"), "one") == 0;
    unsetenv("OS402_TEST");
    return a && b && std::getenv("OS402_TEST") == nullptr;
}

struct Test
{
    const char* id;
    const char* interfaces;
    const char* rule;
    bool (*fn)();
};

} // namespace

int main()
{
    char tmpl[] = "/tmp/os402-XXXXXX";
    if (mkdtemp(tmpl) == nullptr) return 2;
    dir = tmpl;
    const std::vector<Test> tests = {
        {"T01", "snprintf", "truncates, returns the untruncated length", t_snprintf_truncates},
        {"T02", "strtol", "overflow gives LONG_MAX and ERANGE", t_strtol_range},
        {"T03", "open", "O_CREAT|O_EXCL on an existing file fails with EEXIST", t_open_excl},
        {"T04", "open, close", "a new descriptor is the lowest unused number", t_lowest_descriptor},
        {"T05", "pipe, read", "read with no writer left returns 0 (end of file)", t_pipe_eof},
        {"T06", "pipe, write, sigaction", "write with no reader, SIGPIPE ignored: EPIPE", t_pipe_epipe},
        {"T07", "dup2, lseek", "duplicates share one file offset", t_dup2_shares_offset},
        {"T08", "opendir, readdir", "lists the entries, including . and ..", t_readdir},
        {"T09", "clock_gettime", "CLOCK_MONOTONIC never goes backwards", t_monotonic},
        {"T10", "poll", "no event before the timeout: returns 0", t_poll_timeout},
        {"T11", "fork, _exit, waitpid", "exit status 3 is reported as 3", t_wait_status},
        {"T12", "setenv, getenv, unsetenv", "overwrite flag respected; unset removes", t_environment},
    };
    int pass = 0;
    for (const Test& t : tests) {
        bool ok = t.fn();
        pass += ok ? 1 : 0;
        std::printf("%s %-4s %-24s %s\n", t.id, ok ? "PASS" : "FAIL", t.interfaces, t.rule);
    }
    std::printf("summary: %d of %zu passed\n", pass, tests.size());
    std::printf("excluded (not claimed by this subset; each needs the owner's approval):\n");
    std::printf("  locales other than \"C\"/\"POSIX\" - the port provides only the C locale\n");
    std::printf("  termios speed settings - the model terminal has no serial line\n");
    std::printf("  real-time signals and timers - not implemented by the kernel yet\n");
    unlink(path("a").c_str());
    unlink(path("b").c_str());
    rmdir(dir.c_str());
    return pass == static_cast<int>(tests.size()) ? 0 : 1;
}
