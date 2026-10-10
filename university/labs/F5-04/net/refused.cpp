// refused.cpp - what happens when nobody is listening: TCP gets a reset (RST), and a
// connected UDP socket learns about an ICMP "port unreachable" on its next call.
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstring>

struct Fd
{
    int fd;
    explicit Fd(int f) : fd(f) {}
    ~Fd() { if (fd >= 0) { close(fd); } }
    Fd(const Fd&) = delete;
    Fd& operator=(const Fd&) = delete;
};

int main()
{
    sockaddr_in nobody{};
    nobody.sin_family = AF_INET;
    nobody.sin_port = htons(7001);                       // no program listens on 7001
    nobody.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    auto* to = reinterpret_cast<sockaddr*>(&nobody);

    Fd tcp(socket(AF_INET, SOCK_STREAM, 0));
    if (connect(tcp.fd, to, sizeof nobody) != 0) {
        std::printf("TCP connect: %s\n", std::strerror(errno));
    }

    Fd udp(socket(AF_INET, SOCK_DGRAM, 0));
    timeval limit{2, 0};                                 // never wait more than 2 s below
    setsockopt(udp.fd, SOL_SOCKET, SO_RCVTIMEO, &limit, sizeof limit);
    connect(udp.fd, to, sizeof nobody);                  // for UDP this only records the peer
    const ssize_t sent = send(udp.fd, "ping", 4, 0);
    std::printf("UDP send: %zd bytes accepted by the kernel\n", sent);
    char buf[16];
    if (recv(udp.fd, buf, sizeof buf, 0) < 0) {          // the ICMP error arrives here
        std::printf("UDP recv: %s\n", std::strerror(errno));
    }
    return 0;
}
