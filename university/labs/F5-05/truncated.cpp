// truncated.cpp - evidence for the forensic lab "The echo that lost its tail".
// A correct echo server runs in a thread. Jun's client sends one message, then calls
// recv() once and compares what came back with what it sent.
#include "net/socket.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>

#include <cstdio>
#include <functional>
#include <string>
#include <thread>
#include <vector>

void echoForever(const Socket& listener)          // the server: correct, echoes everything
{
    for (;;) {
        const Socket c(::accept(listener.fd(), nullptr, nullptr));
        if (c.fd() < 0) { return; }
        char buf[16 * 1024];
        for (;;) {
            const ssize_t n = ::recv(c.fd(), buf, sizeof buf, 0);
            if (n <= 0) { break; }
            sendAll(c, buf, static_cast<std::size_t>(n));
        }
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
        ::listen(listener.fd(), 4) != 0 ||
        ::getsockname(listener.fd(), reinterpret_cast<sockaddr*>(&a), &len) != 0) {
        throwErrno("listener");
    }
    std::jthread server(echoForever, std::cref(listener));
    const std::string port = std::to_string(ntohs(a.sin_port));

    for (std::size_t size : {5, 1000, 100000, 300000}) {
        const Socket s = connectTo("127.0.0.1", port);
        const std::string message(size, 'm');
        sendAll(s, message.data(), message.size());
        std::vector<char> reply(size);
        const ssize_t n = ::recv(s.fd(), reply.data(), reply.size(), 0);   // Jun's client
        std::printf("sent %6zu bytes, echo received %6zd bytes -> %s\n", size, n,
                    n == static_cast<ssize_t>(size) ? "ok" : "MISMATCH");
    }
    ::shutdown(listener.fd(), SHUT_RDWR);           // wakes the server's accept(): it returns
    return 0;
}
