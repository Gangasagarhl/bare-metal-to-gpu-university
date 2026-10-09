// udp_rtt.cpp - measure round-trip time (RTT) over UDP on the loopback interface.
// An echo thread returns every datagram. The client sends one small datagram, waits for
// the echo, and records how long that took, many times; then it prints the distribution.
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <numeric>
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

double percentile(const std::vector<double>& sorted, double p)   // nearest-rank method
{
    const auto rank = static_cast<std::size_t>(p / 100.0 * static_cast<double>(sorted.size()));
    return sorted[std::min(rank, sorted.size() - 1)];
}

int main()
{
    constexpr int warmup = 1000;
    constexpr int samples = 20000;
    Fd echo(socket(AF_INET, SOCK_DGRAM, 0));
    sockaddr_in a{};
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    socklen_t len = sizeof a;
    bind(echo.fd, reinterpret_cast<sockaddr*>(&a), sizeof a);         // port 0: any free port
    getsockname(echo.fd, reinterpret_cast<sockaddr*>(&a), &len);

    std::jthread server([&echo] {
        for (int i = 0; i < warmup + samples; ++i) {
            char buf[64];
            sockaddr_in from{};
            socklen_t fromLen = sizeof from;
            const ssize_t n = recvfrom(echo.fd, buf, sizeof buf, 0,
                                       reinterpret_cast<sockaddr*>(&from), &fromLen);
            if (n < 0) { return; }
            sendto(echo.fd, buf, static_cast<std::size_t>(n), 0,
                   reinterpret_cast<sockaddr*>(&from), fromLen);
        }
    });

    Fd client(socket(AF_INET, SOCK_DGRAM, 0));
    connect(client.fd, reinterpret_cast<sockaddr*>(&a), sizeof a);
    std::vector<double> rttMicros;
    rttMicros.reserve(samples);
    for (int i = 0; i < warmup + samples; ++i) {
        char msg[32] = "ping";
        const auto t0 = std::chrono::steady_clock::now();
        send(client.fd, msg, sizeof msg, 0);
        recv(client.fd, msg, sizeof msg, 0);
        const auto t1 = std::chrono::steady_clock::now();
        if (i >= warmup) {                                // the first rounds are not counted
            rttMicros.push_back(std::chrono::duration<double, std::micro>(t1 - t0).count());
        }
    }
    std::sort(rttMicros.begin(), rttMicros.end());
    const double mean = std::accumulate(rttMicros.begin(), rttMicros.end(), 0.0) /
                        static_cast<double>(rttMicros.size());
    std::printf("UDP round trips on loopback, 32-byte datagrams: %d samples after %d warm-up\n",
                samples, warmup);
    std::printf("min %.1f us | median %.1f us | p90 %.1f us | p99 %.1f us | p99.9 %.1f us | "
                "max %.1f us | mean %.1f us\n",
                rttMicros.front(), percentile(rttMicros, 50), percentile(rttMicros, 90),
                percentile(rttMicros, 99), percentile(rttMicros, 99.9), rttMicros.back(), mean);
    return 0;
}
