// udp.hpp - F10-31: a small RAII wrapper around a POSIX UDP socket on the loopback
// interface, shared by the vehicle simulator and the companion program.
#pragma once
#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

class UdpSocket {
public:
    explicit UdpSocket(std::uint16_t port)     // port 0 = let the system choose
    {
        fd_ = ::socket(AF_INET, SOCK_DGRAM, 0);
        if (fd_ < 0) {
            std::perror("socket");
            std::exit(2);
        }
        sockaddr_in a{};
        a.sin_family = AF_INET;
        a.sin_port = htons(port);
        a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        if (::bind(fd_, reinterpret_cast<sockaddr*>(&a), sizeof a) != 0) {
            std::perror("bind");
            std::exit(2);
        }
    }
    ~UdpSocket() { ::close(fd_); }
    std::uint16_t port() const                 // the port this socket is bound to
    {
        sockaddr_in a{};
        socklen_t len = sizeof a;
        ::getsockname(fd_, reinterpret_cast<sockaddr*>(&a), &len);
        return ntohs(a.sin_port);
    }
    UdpSocket(const UdpSocket&) = delete;
    UdpSocket& operator=(const UdpSocket&) = delete;

    void sendTo(const std::vector<std::uint8_t>& b, const sockaddr_in& to) const
    {
        ::sendto(fd_, b.data(), b.size(), 0, reinterpret_cast<const sockaddr*>(&to), sizeof to);
    }
    // wait up to timeoutMs for one datagram; returns its bytes (empty if none)
    std::vector<std::uint8_t> receive(int timeoutMs, sockaddr_in& from) const
    {
        pollfd p{fd_, POLLIN, 0};
        if (::poll(&p, 1, timeoutMs) <= 0) {
            return {};
        }
        std::vector<std::uint8_t> buf(512);
        socklen_t len = sizeof from;
        const ssize_t n = ::recvfrom(fd_, buf.data(), buf.size(), 0,
                                     reinterpret_cast<sockaddr*>(&from), &len);
        buf.resize(n > 0 ? static_cast<std::size_t>(n) : 0);
        return buf;
    }

private:
    int fd_ = -1;
};

inline sockaddr_in loopback(std::uint16_t port)
{
    sockaddr_in a{};
    a.sin_family = AF_INET;
    a.sin_port = htons(port);
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    return a;
}
