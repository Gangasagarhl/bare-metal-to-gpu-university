// deadlock.cpp - a client that first sends everything and only then reads the echo.
// With a large enough message both programs end up waiting for each other: the server
// cannot send (the client does not read), so it stops reading; then the client cannot send.
// The lab runs it with a 3-second time limit (deadlock.timeout).
#include "net/socket.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>

#include <cstdio>
#include <functional>
#include <string>
#include <thread>
#include <vector>

void echoOne(const Socket& listener)
{
    const Socket c(::accept(listener.fd(), nullptr, nullptr));
    char buf[16 * 1024];
    for (;;) {
        const ssize_t n = ::recv(c.fd(), buf, sizeof buf, 0);
        if (n <= 0) { return; }
        sendAll(c, buf, static_cast<std::size_t>(n));   // blocks when the client stops reading
    }
}

int main()
{
    Socket listener(::socket(AF_INET, SOCK_STREAM, 0));
    sockaddr_in a{};
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    socklen_t len = sizeof a;
    if (::bind(listener.fd(), reinterpret_cast<sockaddr*>(&a), sizeof a) != 0 ||
        ::listen(listener.fd(), 1) != 0 ||
        ::getsockname(listener.fd(), reinterpret_cast<sockaddr*>(&a), &len) != 0) {
        throwErrno("listener");
    }
    std::jthread server(echoOne, std::cref(listener));
    const Socket s = connectTo("127.0.0.1", std::to_string(ntohs(a.sin_port)));

    const std::vector<char> block(1024 * 1024, 'd');
    std::size_t sent = 0;
    for (int i = 0; i < 64; ++i) {                      // 64 MiB, all before reading
        sendAll(s, block.data(), block.size());
        sent += block.size();
        std::printf("sent %zu MiB so far\n", sent / (1024 * 1024));
        std::fflush(stdout);
    }
    std::printf("all sent; now reading the echo\n");
    return 0;
}
