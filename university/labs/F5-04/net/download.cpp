// download.cpp - a "download": a server thread sends 2 MiB over TCP and the client measures
// how long it takes. With "small", the client asks for a tiny receive buffer and handles the
// data in 1 KiB pieces with a little work (a 200 microsecond pause) after each piece.
// The lab's run.sh decides separately whether some packets are dropped on the way.
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

struct Fd
{
    int fd;
    explicit Fd(int f) : fd(f) {}
    ~Fd() { if (fd >= 0) { close(fd); } }
    Fd(const Fd&) = delete;
    Fd& operator=(const Fd&) = delete;
};

int main(int argc, char** argv)
{
    const bool smallBuffer = argc > 1 && std::string(argv[1]) == "small";
    constexpr std::size_t total = 2 * 1024 * 1024;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(41000);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    Fd listener(socket(AF_INET, SOCK_STREAM, 0));
    const int yes = 1;
    setsockopt(listener.fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
    if (bind(listener.fd, reinterpret_cast<sockaddr*>(&addr), sizeof addr) != 0 ||
        listen(listener.fd, 1) != 0) {
        std::printf("server setup failed: %s\n", std::strerror(errno));
        return 1;
    }
    std::thread server([&listener] {
        Fd conn(accept(listener.fd, nullptr, nullptr));
        const std::vector<char> block(64 * 1024, 'x');
        std::size_t sent = 0;
        while (sent < total) {
            const ssize_t n = send(conn.fd, block.data(), block.size(), 0);
            if (n <= 0) { break; }
            sent += static_cast<std::size_t>(n);
        }
    });

    Fd client(socket(AF_INET, SOCK_STREAM, 0));
    if (smallBuffer) {
        const int small = 4096;                        // must be set before connect()
        setsockopt(client.fd, SOL_SOCKET, SO_RCVBUF, &small, sizeof small);
    }
    int actual = 0;
    socklen_t len = sizeof actual;
    getsockopt(client.fd, SOL_SOCKET, SO_RCVBUF, &actual, &len);

    const auto start = std::chrono::steady_clock::now();
    if (connect(client.fd, reinterpret_cast<sockaddr*>(&addr), sizeof addr) != 0) {
        std::printf("connect failed: %s\n", std::strerror(errno));
        server.join();
        return 1;
    }
    std::vector<char> buf(smallBuffer ? 1024 : 64 * 1024);
    std::size_t got = 0;
    while (got < total) {
        const ssize_t n = recv(client.fd, buf.data(), buf.size(), 0);
        if (n <= 0) { break; }
        got += static_cast<std::size_t>(n);
        if (smallBuffer) { std::this_thread::sleep_for(std::chrono::microseconds(200)); }
    }
    const auto elapsed = std::chrono::steady_clock::now() - start;
    const double s = std::chrono::duration<double>(elapsed).count();
    server.join();
    const double mib = static_cast<double>(got) / (1024.0 * 1024.0);
    std::printf("%s client: %zu bytes in %.3f s = %.1f MiB/s (receive buffer reported: %d bytes)\n",
                smallBuffer ? "small-buffer" : "normal-buffer", got, s, mib / s, actual);
    return got == total ? 0 : 1;
}
