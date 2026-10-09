// bench_tcp.cpp - DS401 F5-37, Listing 1: a perftest-style measurement harness, over TCP.
// The same protocol the perftest tools follow for RDMA (see the chapter): fixed message
// sizes, warm-up iterations that are not timed, many timed iterations, a latency test
// (ping-pong; one-way latency = round trip / 2) and a bandwidth test (one direction,
// many messages in flight; bandwidth = bytes / time), percentiles instead of one average,
// and an alpha-beta fit of the latency curve. Two threads on the loopback interface.
#include <algorithm>
#include <arpa/inet.h>
#include <chrono>
#include <cmath>
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

using Clock = std::chrono::steady_clock;

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
        if (r <= 0) {
            return false;
        }
        p += r;
        n -= static_cast<size_t>(r);
    }
    return true;
}

struct Pair            // one connected TCP pair on loopback: client fd and server fd
{
    int client = -1;
    int server = -1;
    Pair()
    {
        const int l = ::socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in a{};
        a.sin_family = AF_INET;
        a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        socklen_t len = sizeof a;
        if (l < 0 || ::bind(l, reinterpret_cast<sockaddr*>(&a), sizeof a) != 0 || ::listen(l, 1) != 0 ||
            ::getsockname(l, reinterpret_cast<sockaddr*>(&a), &len) != 0) {
            throw std::runtime_error("listen failed");
        }
        client = ::socket(AF_INET, SOCK_STREAM, 0);
        if (::connect(client, reinterpret_cast<sockaddr*>(&a), sizeof a) != 0) {
            throw std::runtime_error("connect failed");
        }
        server = ::accept(l, nullptr, nullptr);
        ::close(l);
        const int one = 1;
        for (const int fd : {client, server}) {
            ::setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof one);
        }
    }
    ~Pair()
    {
        ::close(client);
        ::close(server);
    }
    Pair(const Pair&) = delete;
    Pair& operator=(const Pair&) = delete;
};

double percentile(std::vector<double> v, double q)
{
    std::sort(v.begin(), v.end());
    return v[static_cast<size_t>(q * static_cast<double>(v.size() - 1))];
}

// Latency test: ping-pong of `size` bytes. Returns one-way latencies (round trip / 2) in us.
std::vector<double> latencyTest(size_t size, int warmup, int iters)
{
    Pair p;
    std::thread echo([&] {
        std::vector<char> b(size);
        while (recvAll(p.server, b.data(), size)) {
            sendAll(p.server, b.data(), size);
        }
    });
    std::vector<char> out(size, 'L');
    std::vector<char> in(size);
    std::vector<double> oneWay;
    for (int i = 0; i < warmup + iters; ++i) {
        const auto t0 = Clock::now();
        sendAll(p.client, out.data(), size);
        recvAll(p.client, in.data(), size);
        if (i >= warmup) {
            oneWay.push_back(std::chrono::duration<double, std::micro>(Clock::now() - t0).count() / 2.0);
        }
    }
    ::shutdown(p.client, SHUT_WR);   // the echo thread sees end of stream and stops
    echo.join();
    return oneWay;
}

// Bandwidth test: stream `count` messages of `size` bytes in one direction; the receiver
// sends one byte back at the end, so the time covers delivery of every byte.
double bandwidthTest(size_t size, int count)
{
    Pair p;
    std::thread sink([&] {
        std::vector<char> b(size);
        for (int i = 0; i < count; ++i) {
            recvAll(p.server, b.data(), size);
        }
        const char done = 'D';
        sendAll(p.server, &done, 1);
    });
    std::vector<char> out(size, 'B');
    const auto t0 = Clock::now();
    for (int i = 0; i < count; ++i) {
        sendAll(p.client, out.data(), size);
    }
    char done = 0;
    recvAll(p.client, &done, 1);
    const double s = std::chrono::duration<double>(Clock::now() - t0).count();
    sink.join();
    return static_cast<double>(size) * count / s / 1e6;   // MB/s, decimal megabytes
}

}  // namespace

int main()
{
    try {
        std::printf("perftest-style harness over TCP loopback (DS401 F5-37); all numbers measured in this run\n\n");
        std::printf("latency test: ping-pong, 200 warm-up + 2000 timed iterations, one-way = RTT/2 (us)\n");
        std::printf("%10s %10s %10s %10s %10s\n", "bytes", "t_min", "t_median", "t_p99", "t_max");
        std::vector<double> xs;
        std::vector<double> ys;
        for (const size_t size : {size_t{2}, size_t{64}, size_t{1024}, size_t{4096}, size_t{16384}, size_t{65536}}) {
            const std::vector<double> t = latencyTest(size, 200, 2000);
            std::printf("%10zu %10.2f %10.2f %10.2f %10.2f\n", size, percentile(t, 0.0), percentile(t, 0.5),
                        percentile(t, 0.99), percentile(t, 1.0));
            xs.push_back(static_cast<double>(size));
            ys.push_back(percentile(t, 0.5));
        }
        // Least-squares fit of median one-way latency t(n) = alpha + n / beta.
        const double n = static_cast<double>(xs.size());
        double sx = 0, sy = 0, sxx = 0, sxy = 0;
        for (size_t i = 0; i < xs.size(); ++i) {
            sx += xs[i];
            sy += ys[i];
            sxx += xs[i] * xs[i];
            sxy += xs[i] * ys[i];
        }
        const double slope = (n * sxy - sx * sy) / (n * sxx - sx * sx);   // us per byte
        const double alpha = (sy - slope * sx) / n;
        std::printf("alpha-beta fit of the medians: alpha = %.2f us, 1/beta = %.4f ns/byte (beta = %.0f MB/s)\n\n",
                    alpha, slope * 1000.0, 1.0 / slope);

        std::printf("bandwidth test: one direction, 3 repetitions per size, MB/s = 10^6 bytes/s\n");
        std::printf("%10s %8s %12s %12s %12s %14s\n", "bytes", "msgs", "BW_min", "BW_median", "BW_max", "Mmsg/s(med)");
        for (const size_t size : {size_t{64}, size_t{1024}, size_t{16384}, size_t{65536}, size_t{1048576}}) {
            const int count = static_cast<int>(std::max<size_t>(200, (64u << 20) / size / 4));
            std::vector<double> bw;
            for (int r = 0; r < 3; ++r) {
                bw.push_back(bandwidthTest(size, count));
            }
            std::sort(bw.begin(), bw.end());
            std::printf("%10zu %8d %12.1f %12.1f %12.1f %14.3f\n", size, count, bw[0], bw[1], bw[2],
                        bw[1] / static_cast<double>(size));
        }
    } catch (const std::exception& e) {
        std::printf("error: %s\n", e.what());
        return 1;
    }
    return 0;
}
