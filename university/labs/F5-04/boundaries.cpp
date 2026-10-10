// boundaries.cpp - UDP keeps message boundaries, TCP does not.
// Three short messages are sent over each protocol on the loopback interface; then the
// receiver reads with a large buffer and prints what each read returned.
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <string>
#include <thread>

struct Fd
{
    int fd;
    explicit Fd(int f) : fd(f) {}
    ~Fd() { if (fd >= 0) { close(fd); } }
    Fd(const Fd&) = delete;
    Fd& operator=(const Fd&) = delete;
};

sockaddr_in loopbackAnyPort()
{
    sockaddr_in a{};
    a.sin_family = AF_INET;
    a.sin_port = 0;                                   // 0: let the kernel pick a free port
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    return a;
}

void bindAndName(int fd, sockaddr_in& a)              // bind, then learn the chosen port
{
    socklen_t len = sizeof a;
    bind(fd, reinterpret_cast<sockaddr*>(&a), sizeof a);
    getsockname(fd, reinterpret_cast<sockaddr*>(&a), &len);
}

void readAll(int fd, const char* label, int reads)
{
    for (int i = 0; i < reads; ++i) {
        char buf[256];
        const ssize_t n = recv(fd, buf, sizeof buf, MSG_DONTWAIT);
        if (n <= 0) { break; }
        std::printf("%s read %d: %2zd bytes \"%.*s\"\n", label, i + 1, n, static_cast<int>(n), buf);
    }
}

int main()
{
    const std::string messages[] = {"apple;", "banana;", "cherry;"};

    sockaddr_in ua = loopbackAnyPort();
    Fd urx(socket(AF_INET, SOCK_DGRAM, 0));
    bindAndName(urx.fd, ua);
    Fd utx(socket(AF_INET, SOCK_DGRAM, 0));
    for (const std::string& m : messages) {
        sendto(utx.fd, m.data(), m.size(), 0, reinterpret_cast<sockaddr*>(&ua), sizeof ua);
    }

    sockaddr_in ta = loopbackAnyPort();
    Fd listener(socket(AF_INET, SOCK_STREAM, 0));
    bindAndName(listener.fd, ta);
    listen(listener.fd, 1);
    Fd ttx(socket(AF_INET, SOCK_STREAM, 0));
    connect(ttx.fd, reinterpret_cast<sockaddr*>(&ta), sizeof ta);   // completes in the kernel
    Fd trx(accept(listener.fd, nullptr, nullptr));
    for (const std::string& m : messages) {
        send(ttx.fd, m.data(), m.size(), 0);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));     // let everything arrive
    readAll(urx.fd, "UDP", 5);
    readAll(trx.fd, "TCP", 5);
    return 0;
}
