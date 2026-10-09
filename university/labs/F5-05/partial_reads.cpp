// partial_reads.cpp - TCP is a byte stream: one big send() is not one big recv().
// A thread hands 1 MiB to sendAll() at once; the receiver asks for up to 256 KiB
// per recv() and records how many bytes each call really returned.
#include "net/socket.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>

#include <cstdio>
#include <map>
#include <thread>
#include <vector>

int main()
{
    Socket listener(::socket(AF_INET, SOCK_STREAM, 0));
    sockaddr_in a{};
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);       // port 0: the kernel picks a free one
    socklen_t len = sizeof a;
    if (::bind(listener.fd(), reinterpret_cast<sockaddr*>(&a), sizeof a) != 0 ||
        ::listen(listener.fd(), 1) != 0 ||
        ::getsockname(listener.fd(), reinterpret_cast<sockaddr*>(&a), &len) != 0) {
        throwErrno("listener");
    }
    const std::string port = std::to_string(ntohs(a.sin_port));

    constexpr std::size_t total = 1024 * 1024;
    std::jthread sender([&port] {
        const Socket s = connectTo("127.0.0.1", port);
        const std::vector<char> data(total, 'z');
        sendAll(s, data.data(), data.size());         // one call from our point of view
    });

    const Socket conn(::accept(listener.fd(), nullptr, nullptr));
    std::vector<char> buf(256 * 1024);
    std::map<long, int> sizes;                        // bytes returned -> how many times
    std::size_t got = 0;
    int calls = 0;
    while (got < total) {
        const ssize_t n = ::recv(conn.fd(), buf.data(), buf.size(), 0);
        if (n <= 0) { break; }
        got += static_cast<std::size_t>(n);
        ++calls;
        ++sizes[static_cast<long>(n)];
    }
    std::printf("received %zu bytes in %d recv() calls; sizes returned:\n", got, calls);
    for (const auto& [size, count] : sizes) {
        std::printf("  %7ld bytes x %d\n", size, count);
    }
    return 0;
}
