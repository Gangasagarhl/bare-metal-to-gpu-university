// tcp_throughput.cpp - measure how fast one TCP connection on the loopback interface moves
// data, for several sizes of send() call. Each size is measured 5 times; the median counts.
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <thread>
#include <vector>

struct Fd
{
    int fd;
    explicit Fd(int f) : fd(f) {}
    ~Fd() { if (fd >= 0) { close(fd); } }
    Fd(const Fd&) = delete;
    Fd& operator=(const Fd&) = delete;
};

// Move `total` bytes in send() calls of `chunk` bytes; return MiB per second at the receiver.
double runOnce(std::size_t total, std::size_t chunk)
{
    Fd listener(socket(AF_INET, SOCK_STREAM, 0));
    sockaddr_in a{};
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    socklen_t len = sizeof a;
    bind(listener.fd, reinterpret_cast<sockaddr*>(&a), sizeof a);
    listen(listener.fd, 1);
    getsockname(listener.fd, reinterpret_cast<sockaddr*>(&a), &len);

    std::jthread sender([a, total, chunk] {
        Fd s(socket(AF_INET, SOCK_STREAM, 0));
        if (connect(s.fd, reinterpret_cast<const sockaddr*>(&a), sizeof a) != 0) { return; }
        const std::vector<char> data(chunk, 't');
        for (std::size_t sent = 0; sent < total;) {
            const ssize_t n = send(s.fd, data.data(), std::min(chunk, total - sent), 0);
            if (n <= 0) { return; }
            sent += static_cast<std::size_t>(n);
        }
    });
    Fd conn(accept(listener.fd, nullptr, nullptr));
    std::vector<char> buf(256 * 1024);
    std::size_t got = 0;
    const auto t0 = std::chrono::steady_clock::now();
    while (got < total) {
        const ssize_t n = recv(conn.fd, buf.data(), buf.size(), 0);
        if (n <= 0) { break; }
        got += static_cast<std::size_t>(n);
    }
    const auto elapsed = std::chrono::steady_clock::now() - t0;
    const double sec = std::chrono::duration<double>(elapsed).count();
    return static_cast<double>(got) / (1024.0 * 1024.0) / sec;
}

int main(int argc, char** argv)
{
    const std::size_t mib = argc > 1 ? std::stoul(argv[1]) : 16;   // data per measurement
    std::printf("TCP on loopback, %zu MiB per run, 5 runs per size, MiB/s\n", mib);
    std::printf("%10s %10s %10s %10s\n", "send size", "min", "median", "max");
    for (std::size_t chunk : {64, 1024, 16 * 1024, 256 * 1024}) {
        std::vector<double> r;
        for (int i = 0; i < 5; ++i) {
            r.push_back(runOnce(mib * 1024 * 1024, chunk));
        }
        std::sort(r.begin(), r.end());
        std::printf("%10zu %10.0f %10.0f %10.0f\n", chunk, r.front(), r[2], r.back());
    }
    return 0;
}
