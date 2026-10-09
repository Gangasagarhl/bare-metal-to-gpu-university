// send_one.cpp - send one small UDP datagram over the loopback interface and receive it,
// so that a capture tool can show the real bytes the operating system put around it.
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <string>

struct Fd   // closes the socket when it goes out of scope (RAII, SP102 F2-11)
{
    int fd;
    explicit Fd(int f) : fd(f) {}
    ~Fd() { if (fd >= 0) { close(fd); } }
    Fd(const Fd&) = delete;
    Fd& operator=(const Fd&) = delete;
};

int fail(const char* what)
{
    std::printf("%s failed: %s\n", what, std::strerror(errno));
    return 1;
}

int main()
{
    sockaddr_in to{};
    to.sin_family = AF_INET;
    to.sin_port = htons(5000);                        // port 5000, in network byte order
    to.sin_addr.s_addr = htonl(INADDR_LOOPBACK);      // 127.0.0.1

    Fd rx(socket(AF_INET, SOCK_DGRAM, 0));            // the receiving friend
    if (rx.fd < 0) { return fail("socket"); }
    if (bind(rx.fd, reinterpret_cast<sockaddr*>(&to), sizeof to) < 0) { return fail("bind"); }

    Fd tx(socket(AF_INET, SOCK_DGRAM, 0));            // the sending friend
    if (tx.fd < 0) { return fail("socket"); }
    const std::string text = "Hello, Amara!";
    const ssize_t sent = sendto(tx.fd, text.data(), text.size(), 0,
                                reinterpret_cast<sockaddr*>(&to), sizeof to);
    if (sent < 0) { return fail("sendto"); }
    std::printf("sent %zd bytes of payload\n", sent);

    char buf[100];
    sockaddr_in from{};
    socklen_t fromLen = sizeof from;
    const ssize_t got = recvfrom(rx.fd, buf, sizeof buf, 0,
                                 reinterpret_cast<sockaddr*>(&from), &fromLen);
    if (got < 0) { return fail("recvfrom"); }
    char who[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &from.sin_addr, who, sizeof who);
    std::printf("received %zd bytes from %s port %u: \"%.*s\"\n",
                got, who, ntohs(from.sin_port), static_cast<int>(got), buf);
    return 0;
}
