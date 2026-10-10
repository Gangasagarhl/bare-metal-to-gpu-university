// socket.hpp - a small RAII wrapper around POSIX TCP sockets for the DS201 labs.
// Every failing call throws std::system_error carrying errno, so no error goes unnoticed.
#pragma once

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>

[[noreturn]] inline void throwErrno(const std::string& what)
{
    throw std::system_error(errno, std::generic_category(), what);
}

class Socket
{
public:
    Socket() = default;
    explicit Socket(int fd) : fd_(fd) {}
    ~Socket() { reset(); }
    Socket(const Socket&) = delete;                  // one owner per descriptor
    Socket& operator=(const Socket&) = delete;
    Socket(Socket&& other) noexcept : fd_(std::exchange(other.fd_, -1)) {}
    Socket& operator=(Socket&& other) noexcept
    {
        if (this != &other) {
            reset();
            fd_ = std::exchange(other.fd_, -1);
        }
        return *this;
    }
    int fd() const { return fd_; }
    void reset()
    {
        if (fd_ >= 0) {
            ::close(fd_);
            fd_ = -1;
        }
    }

private:
    int fd_ = -1;
};

// Resolve host and port with getaddrinfo and connect to the first address that works.
inline Socket connectTo(const std::string& host, const std::string& port)
{
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    addrinfo* list = nullptr;
    if (const int rc = getaddrinfo(host.c_str(), port.c_str(), &hints, &list); rc != 0) {
        throw std::runtime_error("getaddrinfo: " + std::string(gai_strerror(rc)));
    }
    int lastErrno = 0;
    for (addrinfo* a = list; a != nullptr; a = a->ai_next) {
        Socket s(::socket(a->ai_family, a->ai_socktype, a->ai_protocol));
        if (s.fd() >= 0 && ::connect(s.fd(), a->ai_addr, a->ai_addrlen) == 0) {
            freeaddrinfo(list);
            return s;
        }
        lastErrno = errno;
    }
    freeaddrinfo(list);
    errno = lastErrno;
    throwErrno("connect to " + host + " port " + port);
}

// Create a listening socket on every local IPv4 address at the given port.
inline Socket listenOn(const std::string& port)
{
    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    addrinfo* list = nullptr;
    if (const int rc = getaddrinfo(nullptr, port.c_str(), &hints, &list); rc != 0) {
        throw std::runtime_error("getaddrinfo: " + std::string(gai_strerror(rc)));
    }
    Socket s(::socket(list->ai_family, list->ai_socktype, list->ai_protocol));
    const int yes = 1;
    ::setsockopt(s.fd(), SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
    const int bound = s.fd() < 0 ? -1 : ::bind(s.fd(), list->ai_addr, list->ai_addrlen);
    freeaddrinfo(list);
    if (bound != 0 || ::listen(s.fd(), 16) != 0) {
        throwErrno("listen on port " + port);
    }
    return s;
}

// send() may accept fewer bytes than offered: loop until everything is handed over.
inline void sendAll(const Socket& s, const char* data, std::size_t size)
{
    while (size > 0) {
        const ssize_t n = ::send(s.fd(), data, size, MSG_NOSIGNAL);
        if (n < 0) {
            if (errno == EINTR) { continue; }
            throwErrno("send");
        }
        data += n;
        size -= static_cast<std::size_t>(n);
    }
}

// recv() may return fewer bytes than asked for: loop until exactly size bytes arrived.
// Returns false if the peer closed the connection first.
inline bool recvExact(const Socket& s, char* data, std::size_t size)
{
    while (size > 0) {
        const ssize_t n = ::recv(s.fd(), data, size, 0);
        if (n == 0) { return false; }
        if (n < 0) {
            if (errno == EINTR) { continue; }
            throwErrno("recv");
        }
        data += n;
        size -= static_cast<std::size_t>(n);
    }
    return true;
}
