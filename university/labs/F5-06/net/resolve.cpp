// resolve.cpp - ask the system's resolver (getaddrinfo) for the IPv4 address of each name
// given on the command line, the way every client program does before it connects.
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <cstdio>

int main(int argc, char** argv)
{
    int failures = 0;
    for (int i = 1; i < argc; ++i) {
        addrinfo hints{};
        hints.ai_family = AF_INET;                 // IPv4 only, to keep the capture short
        hints.ai_socktype = SOCK_STREAM;
        addrinfo* list = nullptr;
        const int rc = getaddrinfo(argv[i], nullptr, &hints, &list);
        if (rc != 0) {
            std::printf("%-24s -> error: %s\n", argv[i], gai_strerror(rc));
            ++failures;
            continue;
        }
        for (const addrinfo* a = list; a != nullptr; a = a->ai_next) {
            char text[INET_ADDRSTRLEN];
            const auto* sin = reinterpret_cast<const sockaddr_in*>(a->ai_addr);
            inet_ntop(AF_INET, &sin->sin_addr, text, sizeof text);
            std::printf("%-24s -> %s\n", argv[i], text);
        }
        freeaddrinfo(list);
    }
    return failures == 0 ? 0 : 1;
}
