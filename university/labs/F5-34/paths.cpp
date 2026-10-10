// paths.cpp - DS401 F5-34, Listing 1.
// The same echo (send N bytes, get the same N bytes back) over two paths between two threads:
//   tcp : a TCP connection over the loopback interface (every transfer is a system call,
//         and the kernel copies the bytes in and out of its socket buffers);
//   shm : a mailbox in memory shared by both threads, polled with atomics (no system call
//         and no kernel on the data path; one memcpy per direction).
// Usage: paths          (both paths)   paths tcp   paths shm   (one path, used by strace)
#include <algorithm>
#include <arpa/inet.h>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace {

constexpr int kWarmup = 200;
constexpr int kIters = 3000;

using Clock = std::chrono::steady_clock;

struct Stats
{
    double minUs, medianUs, p99Us;
};

Stats summarise(std::vector<double> us)
{
    std::sort(us.begin(), us.end());
    const auto at = [&](double q) { return us[static_cast<size_t>(q * static_cast<double>(us.size() - 1))]; };
    return {us.front(), at(0.5), at(0.99)};
}

class Fd
{
public:
    explicit Fd(int fd) : fd_(fd)
    {
        if (fd_ < 0) {
            throw std::runtime_error(std::string("socket call failed: ") + std::strerror(errno));
        }
    }
    ~Fd() { ::close(fd_); }
    Fd(const Fd&) = delete;
    Fd& operator=(const Fd&) = delete;
    int get() const { return fd_; }

private:
    int fd_;
};

void sendAll(int fd, const char* p, size_t n)
{
    while (n > 0) {
        const ssize_t r = ::send(fd, p, n, MSG_NOSIGNAL);
        if (r < 0) {
            throw std::runtime_error(std::string("send: ") + std::strerror(errno));
        }
        p += r;
        n -= static_cast<size_t>(r);
    }
}

bool recvAll(int fd, char* p, size_t n)
{
    while (n > 0) {
        const ssize_t r = ::recv(fd, p, n, 0);
        if (r == 0) {
            return false;  // peer closed
        }
        if (r < 0) {
            throw std::runtime_error(std::string("recv: ") + std::strerror(errno));
        }
        p += r;
        n -= static_cast<size_t>(r);
    }
    return true;
}

// ---- path 1: TCP over loopback --------------------------------------------------------
Stats tcpEcho(size_t size)
{
    Fd listener(::socket(AF_INET, SOCK_STREAM, 0));
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;  // let the kernel choose a free port
    socklen_t len = sizeof addr;
    if (::bind(listener.get(), reinterpret_cast<sockaddr*>(&addr), sizeof addr) != 0 ||
        ::listen(listener.get(), 1) != 0 ||
        ::getsockname(listener.get(), reinterpret_cast<sockaddr*>(&addr), &len) != 0) {
        throw std::runtime_error("bind/listen failed");
    }
    const int one = 1;
    std::thread server([&] {
        Fd s(::accept(listener.get(), nullptr, nullptr));
        ::setsockopt(s.get(), IPPROTO_TCP, TCP_NODELAY, &one, sizeof one);
        std::vector<char> buf(size);
        while (recvAll(s.get(), buf.data(), size)) {
            sendAll(s.get(), buf.data(), size);
        }
    });
    std::vector<double> us;
    {
        Fd c(::socket(AF_INET, SOCK_STREAM, 0));
        if (::connect(c.get(), reinterpret_cast<sockaddr*>(&addr), sizeof addr) != 0) {
            throw std::runtime_error("connect failed");
        }
        ::setsockopt(c.get(), IPPROTO_TCP, TCP_NODELAY, &one, sizeof one);
        std::vector<char> out(size, 'x');
        std::vector<char> in(size);
        for (int i = 0; i < kWarmup + kIters; ++i) {
            const auto t0 = Clock::now();
            sendAll(c.get(), out.data(), size);
            recvAll(c.get(), in.data(), size);
            const auto t1 = Clock::now();
            if (i >= kWarmup) {
                us.push_back(std::chrono::duration<double, std::micro>(t1 - t0).count());
            }
        }
    }  // closing the client ends the server loop
    server.join();
    return summarise(us);
}

// ---- path 2: shared-memory mailbox, polled ---------------------------------------------
struct Mailbox
{
    std::atomic<uint32_t> requestSeq{0};
    std::atomic<uint32_t> replySeq{0};
    std::vector<char> request;
    std::vector<char> reply;
};

Stats shmEcho(size_t size)
{
    Mailbox box;
    box.request.resize(size);
    box.reply.resize(size);
    const uint32_t total = kWarmup + kIters;
    std::thread server([&] {
        for (uint32_t seq = 1; seq <= total; ++seq) {
            while (box.requestSeq.load(std::memory_order_acquire) != seq) {
                // busy poll: no system call, no sleep
            }
            std::memcpy(box.reply.data(), box.request.data(), size);
            box.replySeq.store(seq, std::memory_order_release);
        }
    });
    std::vector<char> out(size, 'x');
    std::vector<char> in(size);
    std::vector<double> us;
    for (uint32_t seq = 1; seq <= total; ++seq) {
        const auto t0 = Clock::now();
        std::memcpy(box.request.data(), out.data(), size);
        box.requestSeq.store(seq, std::memory_order_release);
        while (box.replySeq.load(std::memory_order_acquire) != seq) {
        }
        std::memcpy(in.data(), box.reply.data(), size);
        const auto t1 = Clock::now();
        if (seq > kWarmup) {
            us.push_back(std::chrono::duration<double, std::micro>(t1 - t0).count());
        }
    }
    server.join();
    return summarise(us);
}

void report(const char* path, size_t size, const Stats& s)
{
    std::printf("%-4s %8zu B   min %9.2f us   median %9.2f us   p99 %9.2f us\n",
                path, size, s.minUs, s.medianUs, s.p99Us);
}

}  // namespace

int main(int argc, char** argv)
{
    const std::string only = argc > 1 ? argv[1] : "";
    std::printf("echo round trips between two threads; %d timed iterations after %d warm-up\n",
                kIters, kWarmup);
    try {
        for (const size_t size : {size_t{64}, size_t{65536}}) {
            if (only.empty() || only == "tcp") {
                report("tcp", size, tcpEcho(size));
            }
            if (only.empty() || only == "shm") {
                report("shm", size, shmEcho(size));
            }
        }
    } catch (const std::exception& e) {
        std::printf("error: %s\n", e.what());
        return 1;
    }
    return 0;
}
