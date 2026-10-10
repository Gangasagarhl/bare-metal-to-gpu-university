// echo_server.cpp - a TCP echo server: every byte a client sends comes back to it.
// One thread serves many clients with poll(): it waits until some socket is ready,
// handles exactly what is ready, and never blocks on one slow client.
// Usage: echo_server <port> [number of clients to serve before exiting, 0 = forever]
#include "socket.hpp"

#include <poll.h>

#include <cstdio>
#include <exception>
#include <string>
#include <vector>

struct Client
{
    Socket sock;
    std::string pending;          // received but not yet sent back
    bool peerClosed = false;      // the client said "no more data" (recv returned 0)
};

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::fprintf(stderr, "usage: %s <port> [clients]\n", argv[0]);
        return 2;
    }
    try {
        const Socket listener = listenOn(argv[1]);
        const long limit = argc > 2 ? std::stol(argv[2]) : 0;
        long accepted = 0;
        long finished = 0;
        std::vector<Client> clients;
        std::printf("echo server listening on port %s\n", argv[1]);
        std::fflush(stdout);

        while (limit == 0 || finished < limit) {
            std::vector<pollfd> watch;
            watch.push_back({listener.fd(), POLLIN, 0});
            for (const Client& c : clients) {
                short events = c.peerClosed ? 0 : POLLIN;
                if (!c.pending.empty()) { events |= POLLOUT; }
                watch.push_back({c.sock.fd(), events, 0});
            }
            if (::poll(watch.data(), watch.size(), -1) < 0) { throwErrno("poll"); }

            for (std::size_t i = 1; i < watch.size(); ++i) {
                Client& c = clients[i - 1];
                if (watch[i].revents & (POLLIN | POLLHUP | POLLERR)) {
                    char buf[64 * 1024];
                    const ssize_t n = ::recv(c.sock.fd(), buf, sizeof buf, 0);
                    if (n > 0) {
                        c.pending.append(buf, static_cast<std::size_t>(n));
                    } else {
                        c.peerClosed = true;          // 0 = orderly close; < 0 = error
                    }
                }
                if ((watch[i].revents & POLLOUT) && !c.pending.empty()) {
                    const ssize_t n = ::send(c.sock.fd(), c.pending.data(), c.pending.size(),
                                             MSG_NOSIGNAL);
                    if (n > 0) {
                        c.pending.erase(0, static_cast<std::size_t>(n));
                    } else {
                        c.peerClosed = true;
                        c.pending.clear();
                    }
                }
            }
            // Drop clients that closed and have nothing left to receive from us.
            for (std::size_t i = clients.size(); i-- > 0;) {
                if (clients[i].peerClosed && clients[i].pending.empty()) {
                    clients.erase(clients.begin() + static_cast<std::ptrdiff_t>(i));
                    ++finished;
                }
            }
            if (watch[0].revents & POLLIN) {
                Socket s(::accept(listener.fd(), nullptr, nullptr));
                if (s.fd() >= 0) {
                    clients.push_back({std::move(s), {}, false});
                    ++accepted;
                }
            }
        }
        std::printf("served %ld clients, exiting\n", accepted);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "echo_server: %s\n", e.what());
        return 1;
    }
    return 0;
}
