// launcher.cpp - F9-68: a tiny supervisor that starts real processes in dependency order,
// waits for each to report READY on a pipe, restarts a unit that dies, and stops everything
// in reverse order at the end. The "units" are copies of this program in child mode, so the
// lab needs nothing else. One unit (motor_bridge) is made to crash on its first attempt.
#include <poll.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <thread>
#include <vector>

namespace {

struct Unit {
    std::string name;
    int ready_ms;                    // how long the child needs before it is ready
    std::vector<int> needs;          // indices of units that must be READY first
    int crash_attempts;              // the first N attempts exit with an error before READY
    pid_t pid = -1;
    int fd = -1;                     // read end of the child's READY pipe
    int attempts = 0;
    bool ready = false;
};

using Clock = std::chrono::steady_clock;
const Clock::time_point kT0 = Clock::now();
long ms() { return static_cast<long>(std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - kT0).count()); }

// child mode: initialise for ready_ms, then report on fd 3 (or crash), then wait to be stopped
[[noreturn]] void child(int ready_ms, bool crash)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ready_ms));
    if (crash) {
        _exit(3);                    // e.g. the motor microcontroller did not answer
    }
    const char msg[] = "READY\n";
    if (write(3, msg, sizeof msg - 1) < 0) {
        _exit(4);
    }
    pause();                         // run until SIGTERM
    _exit(0);
}

void launch(Unit& u, const char* self)
{
    int p[2];
    if (pipe(p) != 0) {
        std::perror("pipe");
        std::exit(1);
    }
    ++u.attempts;
    bool crash = u.attempts <= u.crash_attempts;
    pid_t pid = fork();
    if (pid == 0) {
        close(p[0]);
        dup2(p[1], 3);
        std::string ready = std::to_string(u.ready_ms);
        execl(self, self, "--child", ready.c_str(), crash ? "crash" : "ok", static_cast<char*>(nullptr));
        _exit(127);
    }
    close(p[1]);
    u.pid = pid;
    u.fd = p[0];
    std::printf("%5ld ms  launch  %-14s (attempt %d)\n", ms(), u.name.c_str(), u.attempts);
}

}  // namespace

int main(int argc, char** argv)
{
    if (argc == 4 && std::string(argv[1]) == "--child") {
        child(std::atoi(argv[2]), std::string(argv[3]) == "crash");
    }
    std::vector<Unit> units = {
        {"estop_monitor", 20, {}, 0},
        {"imu_driver", 50, {0}, 0},
        {"motor_bridge", 80, {0}, 1},
        {"controller", 30, {1, 2}, 0},
    };
    const int kMaxAttempts = 3;
    bool failed = false;
    while (!failed) {
        bool all_ready = true;
        for (Unit& u : units) {      // launch every unit whose needs are all READY
            all_ready = all_ready && u.ready;
            bool deps = true;
            for (int d : u.needs) {
                deps = deps && units[static_cast<size_t>(d)].ready;
            }
            if (u.pid < 0 && !u.ready && deps) {
                launch(u, argv[0]);
            }
        }
        if (all_ready) {
            break;
        }
        std::vector<pollfd> fds;
        for (Unit& u : units) {
            if (u.fd >= 0) {
                fds.push_back({u.fd, POLLIN, 0});
            }
        }
        poll(fds.data(), fds.size(), 10);
        for (Unit& u : units) {      // READY messages, and children that died
            if (u.fd < 0) {
                continue;
            }
            char buf[16];
            pollfd one{u.fd, POLLIN, 0};
            if (poll(&one, 1, 0) > 0 && (one.revents & POLLIN) && read(u.fd, buf, sizeof buf) > 0) {
                u.ready = true;
                close(u.fd);
                u.fd = -1;
                std::printf("%5ld ms  ready   %s\n", ms(), u.name.c_str());
                continue;
            }
            int status = 0;
            if (waitpid(u.pid, &status, WNOHANG) == u.pid) {
                close(u.fd);
                u.fd = -1;
                u.pid = -1;
                std::printf("%5ld ms  died    %-14s (exit code %d before READY)\n", ms(), u.name.c_str(),
                            WIFEXITED(status) ? WEXITSTATUS(status) : -1);
                if (u.attempts >= kMaxAttempts) {
                    std::printf("%5ld ms  giving up on %s: robot stays in its safe state\n", ms(), u.name.c_str());
                    failed = true;
                } else {
                    std::this_thread::sleep_for(std::chrono::milliseconds(50 * u.attempts));   // back off
                }
            }
        }
    }
    if (!failed) {
        std::printf("%5ld ms  all units READY: the robot may be armed\n", ms());
    }
    for (auto it = units.rbegin(); it != units.rend(); ++it) {   // stop in reverse order
        if (it->pid > 0) {
            kill(it->pid, SIGTERM);
            waitpid(it->pid, nullptr, 0);
            std::printf("%5ld ms  stopped %s\n", ms(), it->name.c_str());
        }
        if (it->fd >= 0) {
            close(it->fd);
        }
    }
    return failed ? 1 : 0;
}
