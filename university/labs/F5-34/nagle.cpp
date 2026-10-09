// nagle.cpp - DS401 F5-34, forensic evidence generator ("we need RDMA: TCP takes 40 ms").
// A tiny request-reply service over TCP on the loopback interface. Each request is written
// as an 8-byte header followed by a 56-byte body; the server answers with an 8-byte reply.
// Usage: nagle            the client exactly as the team ran it (default socket options)
//        nagle nodelay    the client with TCP_NODELAY set
//        nagle onewrite   header and body sent with one writev call (default options)
//        nagle trace      3 requests only, default options (for strace)
#include <algorithm>
#include <arpa/inet.h>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string>
#include <sys/socket.h>
#include <sys/uio.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace {

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

}  // namespace

int main(int argc, char** argv)
{
    const std::string mode = argc > 1 ? argv[1] : "default";
    const int requests = mode == "trace" ? 3 : 50;

    const int listener = ::socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    socklen_t len = sizeof addr;
    if (listener < 0 || ::bind(listener, reinterpret_cast<sockaddr*>(&addr), sizeof addr) != 0 ||
        ::listen(listener, 1) != 0 ||
        ::getsockname(listener, reinterpret_cast<sockaddr*>(&addr), &len) != 0) {
        std::printf("setup failed: %s\n", std::strerror(errno));
        return 1;
    }
    std::thread server([listener] {
        const int s = ::accept(listener, nullptr, nullptr);
        char req[64];
        char rep[8] = {'O', 'K', 0, 0, 0, 0, 0, 0};
        while (recvAll(s, req, sizeof req)) {
            ::send(s, rep, sizeof rep, MSG_NOSIGNAL);
        }
        ::close(s);
    });

    const int c = ::socket(AF_INET, SOCK_STREAM, 0);
    if (::connect(c, reinterpret_cast<sockaddr*>(&addr), sizeof addr) != 0) {
        std::printf("connect failed: %s\n", std::strerror(errno));
        return 1;
    }
    if (mode == "nodelay") {
        const int one = 1;
        ::setsockopt(c, IPPROTO_TCP, TCP_NODELAY, &one, sizeof one);
    }
    int nodelay = -1;
    socklen_t optlen = sizeof nodelay;
    ::getsockopt(c, IPPROTO_TCP, TCP_NODELAY, &nodelay, &optlen);
    std::printf("client config: mode=%s TCP_NODELAY=%d request=8+56 bytes (%s) reply=8 bytes\n",
                mode.c_str(), nodelay, mode == "onewrite" ? "one writev" : "two send calls");

    char header[8] = {'R', 'E', 'Q', 0, 0, 0, 0, 56};
    char body[56] = {};
    char reply[8];
    std::vector<double> ms;
    for (int i = 0; i < requests; ++i) {
        const auto t0 = std::chrono::steady_clock::now();
        if (mode == "onewrite") {
            iovec iov[2] = {{header, sizeof header}, {body, sizeof body}};
            if (::writev(c, iov, 2) != static_cast<ssize_t>(sizeof header + sizeof body)) {
                std::printf("writev failed or was short\n");
                return 1;
            }
        } else {
            ::send(c, header, sizeof header, MSG_NOSIGNAL);
            ::send(c, body, sizeof body, MSG_NOSIGNAL);
        }
        recvAll(c, reply, sizeof reply);
        const auto t1 = std::chrono::steady_clock::now();
        ms.push_back(std::chrono::duration<double, std::milli>(t1 - t0).count());
    }
    ::close(c);
    server.join();
    ::close(listener);

    std::printf("first 8 requests (ms):");
    for (int i = 0; i < std::min(requests, 8); ++i) {
        std::printf(" %.3f", ms[static_cast<size_t>(i)]);
    }
    std::printf("\n");
    std::sort(ms.begin(), ms.end());
    std::printf("%d requests: min %.3f ms, median %.3f ms, max %.3f ms\n", requests, ms.front(),
                ms[ms.size() / 2], ms.back());
    return 0;
}
