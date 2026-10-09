// tcp_talk.cpp - one short TCP conversation on the loopback interface, for capturing:
// connect, one request, one reply, close. The server runs in a second thread.
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <string>
#include <thread>

struct Fd   // closes the socket at the end of its scope
{
    int fd;
    explicit Fd(int f) : fd(f) {}
    ~Fd() { if (fd >= 0) { close(fd); } }
    Fd(const Fd&) = delete;
    Fd& operator=(const Fd&) = delete;
};

bool check(bool ok, const char* what)
{
    if (!ok) { std::printf("%s failed: %s\n", what, std::strerror(errno)); }
    return ok;
}

int main()
{
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(7000);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    Fd listener(socket(AF_INET, SOCK_STREAM, 0));
    const int yes = 1;
    setsockopt(listener.fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
    if (!check(bind(listener.fd, reinterpret_cast<sockaddr*>(&addr), sizeof addr) == 0, "bind") ||
        !check(listen(listener.fd, 1) == 0, "listen")) {
        return 1;
    }

    std::thread server([&listener] {
        Fd conn(accept(listener.fd, nullptr, nullptr));      // waits for the handshake
        char buf[64];
        const ssize_t n = recv(conn.fd, buf, sizeof buf, 0);
        if (n > 0) {
            std::string reply(buf, static_cast<std::size_t>(n));
            for (char& c : reply) { c = static_cast<char>(c >= 'a' && c <= 'z' ? c - 32 : c); }
            send(conn.fd, reply.data(), reply.size(), 0);
        }
        char end[1];
        recv(conn.fd, end, sizeof end, 0);                   // returns 0 when the client closes
    });                                                      // conn closes here: our FIN

    {
        Fd client(socket(AF_INET, SOCK_STREAM, 0));
        if (check(connect(client.fd, reinterpret_cast<sockaddr*>(&addr), sizeof addr) == 0,
                  "connect")) {
            const std::string request = "hello\n";
            send(client.fd, request.data(), request.size(), 0);
            char buf[64];
            const ssize_t n = recv(client.fd, buf, sizeof buf, 0);
            std::printf("client got %zd bytes: %.*s", n, static_cast<int>(n > 0 ? n : 0), buf);
        }
    }                                                        // client closes here: its FIN
    server.join();
    std::printf("conversation over\n");
    return 0;
}
