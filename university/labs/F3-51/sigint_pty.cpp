// sigint_pty.cpp - F3-51: the U1 signal acceptance test, run against the host as the reference.
// "SIGINT from the TTY interrupts a blocked read in a child process, the handler runs, and the
// read returns the interruption error." A pseudo-terminal plays the TTY: the parent types the
// interrupt character (Ctrl-C) on the master side; the terminal's line discipline turns it into
// SIGINT for the child, whose read on the slave side is blocked. Run twice: without and with
// SA_RESTART, which decides whether the interrupted read returns or is restarted.
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

namespace {

volatile sig_atomic_t handler_ran = 0;

void on_sigint(int) { handler_ran = 1; }

// Child: become a session leader, take the slave as controlling terminal, block in read.
[[noreturn]] void child(const char* slave_name, int report_fd, bool restart)
{
    setsid();
    int tty = open(slave_name, O_RDWR);              // first terminal opened: becomes controlling
    if (tty < 0) _exit(10);
    struct sigaction sa {};
    sa.sa_handler = on_sigint;
    sa.sa_flags = restart ? SA_RESTART : 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, nullptr);
    termios t {};
    tcgetattr(tty, &t);
    char line[160];
    int n = std::snprintf(line, sizeof line, "  child: ISIG is %s, VINTR is %d (Ctrl-C), blocking in read\n",
                          (t.c_lflag & ISIG) ? "on" : "off", t.c_cc[VINTR]);
    if (write(report_fd, line, static_cast<size_t>(n)) != n) _exit(11);
    char buf[64];
    errno = 0;
    ssize_t r = read(tty, buf, sizeof buf);          // blocks: nothing typed yet
    int e = errno;
    n = std::snprintf(line, sizeof line, "  child: read returned %zd, errno %d (%s), handler ran: %s\n",
                      r, r < 0 ? e : 0, r < 0 ? std::strerror(e) : "none", handler_ran ? "yes" : "no");
    if (write(report_fd, line, static_cast<size_t>(n)) != n) _exit(11);
    _exit(r < 0 && e == EINTR && handler_ran ? 0 : 1);
}

int run(bool restart)
{
    std::printf("%s:\n", restart ? "with SA_RESTART" : "without SA_RESTART");
    int master = posix_openpt(O_RDWR | O_NOCTTY);
    if (master < 0 || grantpt(master) != 0 || unlockpt(master) != 0) return 2;
    const char* slave = ptsname(master);
    int rep[2];
    if (slave == nullptr || pipe(rep) != 0) return 2;
    std::fflush(stdout);
    pid_t pid = fork();
    if (pid == 0) {
        close(rep[0]);
        child(slave, rep[1], restart);
    }
    close(rep[1]);
    char buf[256];
    ssize_t n = read(rep[0], buf, sizeof buf);       // wait for "blocking in read"
    if (n > 0) std::fwrite(buf, 1, static_cast<size_t>(n), stdout);
    usleep(200 * 1000);                              // give the child time to enter read
    std::printf("  parent: typing Ctrl-C (byte 0x03) on the master side\n");
    if (write(master, "\x03", 1) != 1) return 2;
    if (restart) {
        usleep(200 * 1000);
        std::printf("  parent: typing \"ok\" and Enter, so a restarted read can finish\n");
        if (write(master, "ok\n", 3) != 3) return 2;
    }
    while ((n = read(rep[0], buf, sizeof buf)) > 0) {
        std::fwrite(buf, 1, static_cast<size_t>(n), stdout);
    }
    int st = 0;
    waitpid(pid, &st, 0);
    close(rep[0]);
    close(master);
    int code = WIFEXITED(st) ? WEXITSTATUS(st) : 100;
    if (restart) {
        std::printf("  child exit status %d -> %s\n", code,
                    code == 1 ? "the read was restarted, as SA_RESTART asks" : "unexpected");
    } else {
        std::printf("  child exit status %d -> acceptance test %s\n", code, code == 0 ? "PASS" : "FAIL");
    }
    return code;
}

} // namespace

int main()
{
    int a = run(false);                              // the U1 acceptance test proper
    int b = run(true);                               // the contrast: the read is restarted
    std::printf("U1 signal test (without SA_RESTART): %s\n", a == 0 ? "PASS" : "FAIL");
    return (a == 0 && b == 1) ? 0 : 1;
}
