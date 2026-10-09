// small_requests.cpp - evidence for the forensic lab "Forty milliseconds of nothing".
// A client sends each request as two small writes (a 20-byte header, then an 80-byte body)
// and waits for a 10-byte reply; the server replies once it has the whole 100-byte request.
// The same exchange is timed twice: with the socket's default options, and with
// TCP_NODELAY set on the client socket.
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
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

bool recvExact(int fd, char* p, std::size_t n)
{
    while (n > 0) {
        const ssize_t got = recv(fd, p, n, 0);
        if (got <= 0) { return false; }
        p += got;
        n -= static_cast<std::size_t>(got);
    }
    return true;
}

void measure(bool noDelay, int requests)
{
    Fd listener(socket(AF_INET, SOCK_STREAM, 0));
    sockaddr_in a{};
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    socklen_t len = sizeof a;
    bind(listener.fd, reinterpret_cast<sockaddr*>(&a), sizeof a);
    listen(listener.fd, 1);
    getsockname(listener.fd, reinterpret_cast<sockaddr*>(&a), &len);

    std::jthread server([&listener] {
        Fd c(accept(listener.fd, nullptr, nullptr));
        char request[100];
        while (recvExact(c.fd, request, sizeof request)) {      // whole request first
            send(c.fd, "reply-ok!\n", 10, 0);
        }
    });

    Fd s(socket(AF_INET, SOCK_STREAM, 0));
    const int flag = noDelay ? 1 : 0;
    setsockopt(s.fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof flag);
    connect(s.fd, reinterpret_cast<sockaddr*>(&a), sizeof a);
    std::vector<double> ms;
    const char header[20] = "HDR";
    const char body[80] = "BODY";
    for (int i = 0; i < requests; ++i) {
        const auto t0 = std::chrono::steady_clock::now();
        send(s.fd, header, sizeof header, 0);                   // first small write
        send(s.fd, body, sizeof body, 0);                       // second small write
        char reply[10];
        recvExact(s.fd, reply, sizeof reply);
        const auto t1 = std::chrono::steady_clock::now();
        ms.push_back(std::chrono::duration<double, std::milli>(t1 - t0).count());
    }
    shutdown(s.fd, SHUT_WR);                                    // lets the server loop end
    std::sort(ms.begin(), ms.end());
    std::printf("%-22s %d requests: min %7.3f ms | median %7.3f ms | max %7.3f ms\n",
                noDelay ? "TCP_NODELAY set:" : "default options:", requests, ms.front(),
                ms[ms.size() / 2], ms.back());
}

int main()
{
    measure(false, 50);
    measure(true, 50);
    return 0;
}
