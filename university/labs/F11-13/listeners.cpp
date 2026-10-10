// F11-13 Listing 2: list entry points from the machine itself, not from the manual.
// The program opens a TCP listening socket that nobody "documented" (on 127.0.0.1, a
// port chosen by the kernel), then reads the kernel's own table of TCP sockets,
// /proc/net/tcp, and looks for listening entries. Linux only. To keep the output the
// same on every machine, it prints only the entry it opened, and whether it was found.
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

int main()
{
    const int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        std::perror("socket");
        return 1;
    }
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;    // let the kernel choose a free port
    socklen_t len = sizeof addr;
    if (bind(fd, reinterpret_cast<sockaddr*>(&addr), len) != 0 || listen(fd, 1) != 0 ||
        getsockname(fd, reinterpret_cast<sockaddr*>(&addr), &len) != 0) {
        std::perror("bind/listen");
        close(fd);
        return 1;
    }
    const unsigned port = ntohs(addr.sin_port);
    std::printf("opened an undocumented listener on 127.0.0.1 (port chosen by the kernel)\n");

    std::ifstream tcp("/proc/net/tcp");
    std::string line;
    std::getline(tcp, line);    // header line
    int listening = 0;
    bool found = false;
    while (std::getline(tcp, line)) {
        std::istringstream in(line);
        std::string slot, local, remote, state;
        in >> slot >> local >> remote >> state;
        if (state != "0A") {    // 0A is the listening state in this table
            continue;
        }
        ++listening;
        const std::size_t colon = local.find(':');
        const unsigned p = static_cast<unsigned>(std::stoul(local.substr(colon + 1), nullptr, 16));
        if (p == port && local.substr(0, colon) == "0100007F") {
            found = true;
            std::printf("found in /proc/net/tcp: local address 0100007F (127.0.0.1), "
                        "state 0A (listen)\n");
        }
    }
    std::printf("our listener found by the inventory: %s\n", found ? "yes" : "NO");
    std::printf("other listening TCP sockets on this machine: %s\n",
                listening > 1 ? "some (not printed; the number varies by machine)" : "none");
    close(fd);
    return found ? 0 : 1;
}
