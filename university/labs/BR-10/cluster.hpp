// cluster.hpp - BR-10's three-node virtual cluster, small enough to read in one sitting.
// Each "node" is a separate process with its own address space, reached only through a
// TCP connection on the loopback interface. A controller hands out tasks of the job
// (job.hpp), collects the answers, and writes a structured log of what it saw.
// Faults are injected on purpose, per node: a stall, a sudden death, a wrong clock,
// a pinned CPU. This is the university's teaching model, not Slurm (see the chapter).
#pragma once
#include "job.hpp"
#include <algorithm>
#include <arpa/inet.h>
#include <chrono>
#include <csignal>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <netinet/in.h>
#include <poll.h>
#include <sched.h>
#include <string>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace mini
{

enum class Assign { dynamic, fixed };  // pull the next task, or task t always to node t % n

struct NodeSetup
{
    int stallOnTask = -1;    // stall when this node receives its k-th task (counted from 0)
    int stallMs = 0;         // for how long; -1 means forever
    int dieOnTask = -1;      // the node kills itself when it receives its k-th task
    long clockOffsetMs = 0;  // injected error of this node's wall clock
    int queueMs = 0;         // every task waits this long before the node starts it
                             // (a stand-in for waiting for a CPU on a shared node)
    int cpu = -1;            // pin the node to this CPU (-1: let the OS choose)
};

struct Config
{
    std::uint64_t jobEnd = 300000;
    int tasks = 12;
    Assign assign = Assign::dynamic;
    int timeoutMs = 0;       // 0: wait for a reply forever
    int drainMs = 0;         // after the job: how long to listen for late replies
    bool log = true;
    std::vector<NodeSetup> nodes = std::vector<NodeSetup>(3);
};

struct NodeStats
{
    int tasks = 0;            // tasks this node finished in the last job
    double busyMs = 0;        // sum of its task durations, measured on its own clock
    double replyMs = 0;       // sum of send-to-answer times, measured by the controller
};

struct JobResult
{
    job::Part total;
    double seconds = 0;
    bool finished = false;
    std::vector<NodeStats> perNode;
};

inline long long wallUs()  // the machine's wall clock (system_clock), in microseconds
{
    using namespace std::chrono;
    return duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();
}

inline bool sendAll(int fd, std::string const& s)
{
    std::size_t done = 0;
    while (done < s.size()) {
        ssize_t const n = ::send(fd, s.data() + done, s.size() - done, MSG_NOSIGNAL);
        if (n <= 0) {
            return false;
        }
        done += static_cast<std::size_t>(n);
    }
    return true;
}

struct LineReader  // TCP is a byte stream: collect bytes until a whole line has arrived
{
    int fd = -1;
    std::string buf;
    bool fill()  // false on end of stream or error
    {
        char tmp[512];
        ssize_t const n = ::read(fd, tmp, sizeof tmp);
        if (n <= 0) {
            return false;
        }
        buf.append(tmp, static_cast<std::size_t>(n));
        return true;
    }
    bool next(std::string& line)
    {
        auto const pos = buf.find('\n');
        if (pos == std::string::npos) {
            return false;
        }
        line = buf.substr(0, pos);
        buf.erase(0, pos + 1);
        return true;
    }
};

inline void need(bool ok, char const* what)  // a failed set-up call ends the program loudly
{
    if (!ok) {
        std::perror(what);
        std::exit(2);
    }
}

inline void pinTo(int cpu)
{
    if (cpu >= 0) {
        cpu_set_t set;
        CPU_ZERO(&set);
        CPU_SET(cpu, &set);
        sched_setaffinity(0, sizeof set, &set);
    }
}

// The node agent: runs in the child process, serves one controller connection.
[[noreturn]] inline void nodeMain(int listenFd, NodeSetup const& s)
{
    prctl(PR_SET_PDEATHSIG, SIGKILL);  // never outlive the controller
    pinTo(s.cpu);
    LineReader in{::accept(listenFd, nullptr, nullptr), {}};
    ::close(listenFd);
    int received = 0;
    std::string line;
    while (true) {
        while (!in.next(line)) {
            if (!in.fill()) {
                _exit(0);
            }
        }
        unsigned long long id = 0, lo = 0, hi = 0;
        if (std::sscanf(line.c_str(), "TASK %llu %llu %llu", &id, &lo, &hi) == 3) {
            int const k = received++;
            if (k == s.dieOnTask) {
                ::raise(SIGKILL);  // sudden death: no goodbye, no flushed buffers
            }
            if (s.queueMs > 0) {
                ::usleep(static_cast<useconds_t>(s.queueMs) * 1000);
            }
            long long const start = wallUs() + s.clockOffsetMs * 1000;
            if (k == s.stallOnTask) {
                if (s.stallMs < 0) {
                    while (true) {
                        ::pause();  // stuck forever, but the connection stays open
                    }
                }
                ::usleep(static_cast<useconds_t>(s.stallMs) * 1000);
            }
            job::Part const p = job::countPrimes(lo, hi);
            long long const end = wallUs() + s.clockOffsetMs * 1000;
            sendAll(in.fd, "DONE " + std::to_string(id) + " " + std::to_string(p.count) + " " +
                               std::to_string(p.sum) + " " + std::to_string(start) + " " +
                               std::to_string(end) + "\n");
        } else if (line == "TIME") {
            sendAll(in.fd, "NOW " + std::to_string(wallUs() + s.clockOffsetMs * 1000) + "\n");
        } else {  // BYE
            _exit(0);
        }
    }
}

class Cluster
{
public:
    explicit Cluster(Config cfg) : cfg_(std::move(cfg)), t0_(std::chrono::steady_clock::now())
    {
        std::vector<int> mine;  // controller-side sockets a later child must not keep
        for (std::size_t i = 0; i < cfg_.nodes.size(); ++i) {
            int const lfd = ::socket(AF_INET, SOCK_STREAM, 0);
            need(lfd >= 0, "socket");
            sockaddr_in a{};
            a.sin_family = AF_INET;
            a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);  // 127.0.0.1, port chosen by the OS
            socklen_t len = sizeof a;
            need(::bind(lfd, reinterpret_cast<sockaddr*>(&a), len) == 0, "bind");
            need(::listen(lfd, 1) == 0, "listen");
            need(::getsockname(lfd, reinterpret_cast<sockaddr*>(&a), &len) == 0, "getsockname");
            std::fflush(stdout);
            pid_t const pid = ::fork();
            need(pid >= 0, "fork");
            if (pid == 0) {
                for (int fd : mine) {
                    ::close(fd);
                }
                nodeMain(lfd, cfg_.nodes[i]);
            }
            int const cfd = ::socket(AF_INET, SOCK_STREAM, 0);
            need(cfd >= 0 && ::connect(cfd, reinterpret_cast<sockaddr*>(&a), len) == 0, "connect");
            ::close(lfd);
            mine.push_back(cfd);
            nodes_.push_back(Node{pid, LineReader{cfd, {}}, ntohs(a.sin_port)});
            log("event=node_up node=n%zu pid=%d port=%u", i + 1, static_cast<int>(pid),
                static_cast<unsigned>(nodes_.back().port));
        }
    }

    ~Cluster()  // shut down: goodbye to healthy nodes, fence suspected ones
    {
        for (std::size_t i = 0; i < nodes_.size(); ++i) {
            Node& n = nodes_[i];
            if (n.alive && n.suspected) {
                ::kill(n.pid, SIGKILL);
                log("event=fence node=n%zu how=SIGKILL (stand-in for a power-off through "
                    "its BMC)", i + 1);
            } else if (n.alive) {
                sendAll(n.in.fd, "BYE\n");
            }
            ::close(n.in.fd);
            int st = 0;
            ::waitpid(n.pid, &st, 0);
            if (WIFSIGNALED(st)) {
                log("event=node_exit node=n%zu signal=%d", i + 1, WTERMSIG(st));
            } else {
                log("event=node_exit node=n%zu status=%d", i + 1, WEXITSTATUS(st));
            }
        }
    }

    Cluster(Cluster const&) = delete;
    Cluster& operator=(Cluster const&) = delete;

    void setLog(bool on) { cfg_.log = on; }

    // Ask a node for its clock 'probes' times; keep the probe with the shortest round
    // trip and estimate the offset as node time minus the midpoint of that round trip.
    // Result and error bound (half the round trip) in microseconds.
    long long estimateOffsetUs(std::size_t i, int probes, long long& halfRttUs)
    {
        long long best = -1, offset = 0;
        for (int p = 0; p < probes; ++p) {
            long long const sent = wallUs();
            sendAll(nodes_[i].in.fd, "TIME\n");
            std::string line;
            while (!nodes_[i].in.next(line)) {
                if (!nodes_[i].in.fill()) {
                    halfRttUs = -1;  // the node is gone: no estimate
                    return 0;
                }
            }
            long long const back = wallUs();
            long long nodeNow = 0;
            std::sscanf(line.c_str(), "NOW %lld", &nodeNow);
            if (best < 0 || back - sent < best) {
                best = back - sent;
                offset = nodeNow - (sent + back) / 2;
            }
        }
        halfRttUs = (best + 1) / 2;
        return offset;
    }

    // One run of the whole job. Records go to 'events' for clocks.cpp to study.
    struct Event
    {
        int task;
        std::size_t node;
        long long assignedWall;  // controller's wall clock when the task was sent (us)
        long long receivedWall;  // controller's wall clock when the answer arrived (us)
        long long nodeStart;     // node's wall clock when it started the task (us)
        long long nodeEnd;       // node's wall clock when it finished (us)
    };
    std::vector<Event> events;

    JobResult runJob()
    {
        using clock = std::chrono::steady_clock;
        events.clear();
        int const T = cfg_.tasks;
        std::size_t const N = nodes_.size();
        std::deque<int> pending;
        for (int t = 0; t < T; ++t) {
            pending.push_back(t);
        }
        std::vector<bool> done(static_cast<std::size_t>(T), false);
        std::vector<long long> sentWall(static_cast<std::size_t>(T), 0);
        JobResult r;
        r.perNode.resize(N);
        int doneCount = 0;
        auto const begin = clock::now();
        log("event=job_start tasks=%d range=[0,%llu) assign=%s timeout_ms=%d", T,
            static_cast<unsigned long long>(cfg_.jobEnd),
            cfg_.assign == Assign::dynamic ? "dynamic" : "fixed", cfg_.timeoutMs);

        while (doneCount < T) {
            // 1. Give work to every healthy idle node.
            for (std::size_t i = 0; i < N; ++i) {
                Node& n = nodes_[i];
                if (!n.alive || n.suspected || n.busyTask >= 0) {
                    continue;
                }
                auto it = pending.begin();
                if (cfg_.assign == Assign::fixed) {  // own tasks, or orphans of a lost node
                    while (it != pending.end() && static_cast<std::size_t>(*it) % N != i &&
                           healthy(static_cast<std::size_t>(*it) % N)) {
                        ++it;
                    }
                }
                if (it == pending.end()) {
                    continue;
                }
                int const t = *it;
                pending.erase(it);
                std::uint64_t const lo = job::taskBegin(t, T, cfg_.jobEnd);
                std::uint64_t const hi = job::taskBegin(t + 1, T, cfg_.jobEnd);
                n.busyTask = t;
                n.sentAt = clock::now();
                sentWall[static_cast<std::size_t>(t)] = wallUs();
                sendAll(n.in.fd, "TASK " + std::to_string(t) + " " + std::to_string(lo) + " " +
                                     std::to_string(hi) + "\n");
                log("event=assign task=%d node=n%zu", t, i + 1);
            }
            if (!anyHealthy()) {
                log("event=job_failed reason=no_healthy_node done=%d of %d", doneCount, T);
                r.seconds = std::chrono::duration<double>(clock::now() - begin).count();
                return r;
            }
            // 2. Wait for an answer, a closed connection, or the nearest deadline.
            int waitMs = -1;  // -1: poll waits without limit
            if (cfg_.timeoutMs > 0) {
                for (Node const& n : nodes_) {
                    if (n.alive && !n.suspected && n.busyTask >= 0) {
                        auto const left = cfg_.timeoutMs - msSince(n.sentAt);
                        int const l = left < 0 ? 0 : static_cast<int>(left);
                        waitMs = waitMs < 0 ? l : std::min(waitMs, l);
                    }
                }
            }
            waitForNodes(waitMs, done, sentWall, doneCount, pending, r);
            // 3. Deadlines: a node that has not answered in time is suspected, not proven dead.
            if (cfg_.timeoutMs > 0) {
                for (std::size_t i = 0; i < N; ++i) {
                    Node& n = nodes_[i];
                    if (n.alive && !n.suspected && n.busyTask >= 0 &&
                        msSince(n.sentAt) >= cfg_.timeoutMs) {
                        log("event=timeout node=n%zu task=%d waited_ms=%d action=suspect_node,"
                            "requeue_task", i + 1, n.busyTask, cfg_.timeoutMs);
                        n.suspected = true;
                        n.lateTask = n.busyTask;
                        pending.push_front(n.busyTask);
                        n.busyTask = -1;
                    }
                }
            }
        }
        r.seconds = std::chrono::duration<double>(clock::now() - begin).count();
        r.finished = true;
        log("event=job_done count=%llu sum=%llu", static_cast<unsigned long long>(r.total.count),
            static_cast<unsigned long long>(r.total.sum));
        // 4. Listen a little longer: a suspected node may still answer.
        auto const drainStart = clock::now();
        while (cfg_.drainMs > 0 && anyLateAnswerPossible() &&
               msSince(drainStart) < cfg_.drainMs) {
            waitForNodes(static_cast<int>(cfg_.drainMs - msSince(drainStart)), done, sentWall,
                         doneCount, pending, r);
        }
        return r;
    }

private:
    struct Node
    {
        pid_t pid;
        LineReader in;
        unsigned port;
        bool alive = true;
        bool suspected = false;
        int busyTask = -1;
        int lateTask = -1;  // the task a suspected node may still answer
        std::chrono::steady_clock::time_point sentAt{};
    };

    Config cfg_;
    std::chrono::steady_clock::time_point t0_;
    std::vector<Node> nodes_;

    long long msSince(std::chrono::steady_clock::time_point t) const
    {
        using namespace std::chrono;
        return duration_cast<milliseconds>(steady_clock::now() - t).count();
    }

    bool healthy(std::size_t i) const { return nodes_[i].alive && !nodes_[i].suspected; }

    bool anyHealthy() const
    {
        for (std::size_t i = 0; i < nodes_.size(); ++i) {
            if (healthy(i)) {
                return true;
            }
        }
        return false;
    }

    bool anyLateAnswerPossible() const
    {
        for (Node const& n : nodes_) {
            if (n.alive && n.suspected && n.lateTask >= 0) {
                return true;
            }
        }
        return false;
    }

    void waitForNodes(int waitMs, std::vector<bool>& done, std::vector<long long> const& sentWall,
                      int& doneCount, std::deque<int>& pending, JobResult& r)
    {
        std::vector<pollfd> fds;
        std::vector<std::size_t> who;
        for (std::size_t i = 0; i < nodes_.size(); ++i) {
            if (nodes_[i].alive) {
                fds.push_back(pollfd{nodes_[i].in.fd, POLLIN, 0});
                who.push_back(i);
            }
        }
        std::fflush(stdout);
        if (::poll(fds.data(), fds.size(), waitMs) <= 0) {
            return;
        }
        for (std::size_t k = 0; k < fds.size(); ++k) {
            if (fds[k].revents == 0) {
                continue;
            }
            std::size_t const i = who[k];
            Node& n = nodes_[i];
            if (!n.in.fill()) {  // connection closed: the process is gone
                n.alive = false;
                log("event=node_lost node=n%zu evidence=connection_closed task=%d action=%s",
                    i + 1, n.busyTask, n.busyTask >= 0 ? "requeue_task" : "none");
                if (n.busyTask >= 0 && !done[static_cast<std::size_t>(n.busyTask)]) {
                    pending.push_front(n.busyTask);
                }
                n.busyTask = -1;
                continue;
            }
            std::string line;
            while (n.in.next(line)) {
                long long const now = wallUs();
                unsigned long long id = 0, c = 0, s = 0;
                long long st = 0, en = 0;
                if (std::sscanf(line.c_str(), "DONE %llu %llu %llu %lld %lld", &id, &c, &s, &st,
                                &en) != 5) {
                    continue;
                }
                std::size_t const t = static_cast<std::size_t>(id);
                double const replyMs = std::chrono::duration<double, std::milli>(
                                           std::chrono::steady_clock::now() - n.sentAt).count();
                if (n.busyTask == static_cast<int>(t)) {
                    n.busyTask = -1;
                }
                if (n.lateTask == static_cast<int>(t)) {
                    n.lateTask = -1;
                }
                if (done[t]) {
                    log("event=late_reply node=n%zu task=%llu action=ignored (already done)",
                        i + 1, id);
                    continue;
                }
                done[t] = true;
                ++doneCount;
                r.total.count += c;
                r.total.sum += s;
                r.perNode[i].tasks += 1;
                r.perNode[i].busyMs += static_cast<double>(en - st) / 1000.0;
                r.perNode[i].replyMs += replyMs;
                events.push_back(Event{static_cast<int>(t), i, sentWall[t], now, st, en});
                log("event=done task=%llu node=n%zu node_ms=%.1f", id, i + 1,
                    static_cast<double>(en - st) / 1000.0);
            }
        }
    }

    [[gnu::format(printf, 2, 3)]] void log(char const* fmt, ...) const
    {
        if (!cfg_.log) {
            return;
        }
        double const ms =
            std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0_)
                .count();
        std::printf("t=%8.1fms ", ms);
        va_list ap;
        va_start(ap, fmt);
        std::vprintf(fmt, ap);
        va_end(ap);
        std::printf("\n");
        std::fflush(stdout);
    }
};

}  // namespace mini
